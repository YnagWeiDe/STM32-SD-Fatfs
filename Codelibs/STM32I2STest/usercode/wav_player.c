/* wav_player.c */
/**
 * @file wav_player.c
 * @brief WAV Player Library Implementation
 * 
 * Implements the functions declared in wav_player.h.
 * - Dynamic header parsing: Reads chunks in a loop until 'data' is found, handles extra params.
 * - Supports 1 or 2 channels (mono duplicated to stereo for I2S).
 * - Dynamic I2S reconfiguration for sample rate (deinit/init with new AudioFreq).
 * - Pause/resume: Saves position, stops/resumes DMA.
 * - Debugging: Detailed prints for each step, errors, and progress.
 * - Low coupling: File ops via FatFS/SDFaths, I2S via HAL.
 * - Error handling: Specific codes, safe cleanup on failure.
 * 
 * Note: Assumes 16-bit PCM. Extend for 8/24-bit if needed.
 *       Sample rate config assumes standard PLL settings; test hardware-specific rates.
 *       Buffer size must be multiple of block_align for alignment.
 */

#include "wav_player.h"

// Global player instance (for callbacks; single instance assumed)
static WAV_Player_t *g_player = NULL;

// Helper: Read 4-byte chunk ID (big-endian assumed, but WAV is little-endian; use as is)
static uint32_t read_chunk_id(FIL *file) {
    uint32_t id;
    UINT br;
    f_read(file, &id, 4, &br);
    if (br != 4) {
        printf("Debug: Failed to read chunk ID.\n");
    }
    return id;
}

// Helper: Skip chunk data
static void skip_chunk(FIL *file, uint32_t size) {
    FRESULT res = f_lseek(file, f_tell(file) + size);
    if (res != FR_OK) {
        printf("Debug: Skip chunk failed: %d\n", res);
    }
}

// Dynamic parse WAV header (improved: handles extra chunks like 'LIST', 'FACT')
static int parse_wav_header(WAV_Player_t *player) {
    UINT br;
    FRESULT res;

    // Read RIFF header
    res = f_read(&player->file, &player->header.chunk_id, 4, &br);
    if (res != FR_OK || br != 4 || player->header.chunk_id != 0x46464952) { // 'RIFF' (little-endian)
        printf("Debug: Invalid RIFF header (ID: 0x%08X).\n", player->header.chunk_id);
        return WAV_ERR_INVALID_HEADER;
    }
    f_read(&player->file, &player->header.chunk_size, 4, &br);
    f_read(&player->file, &player->header.format, 4, &br);
    if (player->header.format != 0x45564157) { // 'WAVE'
        printf("Debug: Invalid WAVE format (0x%08X).\n", player->header.format);
        return WAV_ERR_INVALID_HEADER;
    }
    printf("Debug: Valid RIFF WAVE header, total size: %u bytes.\n", player->header.chunk_size + 8);

    int fmt_found = 0;
    while (1) {
        uint32_t subchunk_id = read_chunk_id(&player->file);
        uint32_t subchunk_size;
        f_read(&player->file, &subchunk_size, 4, &br);
        if (br != 4) {
            printf("Debug: Failed to read subchunk size.\n");
            return WAV_ERR_INVALID_HEADER;
        }

        if (subchunk_id == 0x20746D66) { // 'fmt '
            if (fmt_found) {
                printf("Debug: Duplicate 'fmt ' chunk.\n");
                return WAV_ERR_INVALID_HEADER;
            }
            player->header.subchunk1_id = subchunk_id;
            player->header.subchunk1_size = subchunk_size;
            f_read(&player->file, &player->header.audio_format, 2, &br);
            f_read(&player->file, &player->header.num_channels, 2, &br);
            f_read(&player->file, &player->header.sample_rate, 4, &br);
            f_read(&player->file, &player->header.byte_rate, 4, &br);
            f_read(&player->file, &player->header.block_align, 2, &br);
            f_read(&player->file, &player->header.bits_per_sample, 2, &br);

            // Skip extra format bytes (e.g., ext format for >16)
            if (subchunk_size > 16) {
                uint16_t extra_size;
                f_read(&player->file, &extra_size, 2, &br);
                skip_chunk(&player->file, subchunk_size - 18); // Already read 16 + 2
                printf("Debug: Skipped extra fmt params: %u bytes.\n", extra_size);
            }

            // Validate
            if (player->header.audio_format != 1) {
                printf("Debug: Non-PCM format (%u) not supported.\n", player->header.audio_format);
                return WAV_ERR_UNSUPPORTED;
            }
            if (player->header.num_channels != 1 && player->header.num_channels != 2) {
                printf("Debug: Unsupported channels: %u\n", player->header.num_channels);
                return WAV_ERR_UNSUPPORTED;
            }
            if (player->header.bits_per_sample != 16) {
                printf("Debug: Unsupported bit depth: %u\n", player->header.bits_per_sample);
                return WAV_ERR_UNSUPPORTED;
            }
            if (player->header.byte_rate != player->header.sample_rate * player->header.num_channels * (player->header.bits_per_sample / 8)) {
                printf("Debug: Invalid byte rate.\n");
                return WAV_ERR_INVALID_HEADER;
            }
            printf("Debug: FMT - Channels: %u, Sample Rate: %u Hz, Bit Depth: %u, Byte Rate: %u\n",
                   player->header.num_channels, player->header.sample_rate, player->header.bits_per_sample, player->header.byte_rate);
            fmt_found = 1;
        } else if (subchunk_id == 0x61746164) { // 'data'
            if (!fmt_found) {
                printf("Debug: 'data' before 'fmt '.\n");
                return WAV_ERR_INVALID_HEADER;
            }
            player->header.subchunk2_id = subchunk_id;
            player->header.subchunk2_size = subchunk_size;
            player->header.data_offset = f_tell(&player->file);
            player->bytes_remaining = subchunk_size;
            printf("Debug: DATA chunk found, size: %u bytes, offset: %u\n",
                   subchunk_size, player->header.data_offset);
            break;
        } else {
            // Skip unknown chunk (e.g., 'LIST', 'INFO')
            printf("Debug: Skipping unknown chunk: 0x%08X, size: %u\n", subchunk_id, subchunk_size);
            skip_chunk(&player->file, subchunk_size);
        }

        if (f_eof(&player->file)) {
            printf("Debug: EOF without 'data' chunk.\n");
            return WAV_ERR_INVALID_HEADER;
        }
    }

    return WAV_ERR_NONE;
}

// Fill buffer from file (improved: handles partial reads, alignment)
static int fill_buffer(WAV_Player_t *player, uint8_t *buf, uint32_t buf_size) {
    if (player->bytes_remaining == 0) {
        printf("Debug: No bytes remaining.\n");
        return 0;
    }

    UINT br;
    uint32_t to_read = (player->bytes_remaining < buf_size) ? player->bytes_remaining : buf_size;
    // Align to block_align if needed (but for simplicity, assume buffer_size % block_align == 0)
    to_read -= (to_read % player->header.block_align);

    FRESULT res = f_read(&player->file, buf, to_read, &br);
    if (res != FR_OK || br != to_read) {
        printf("Debug: File read error. Res: %d, Read: %u/%u\n", res, br, to_read);
        return WAV_ERR_READ_FAIL;
    }
    player->bytes_remaining -= br;
    player->current_position += br;

    // If mono, duplicate to stereo (in-place, backwards to avoid overlap)
    if (player->header.num_channels == 1 && player->header.bits_per_sample == 16) {
        int16_t *samples = (int16_t *)buf;
        uint32_t num_samples = br / 2;
        for (int i = num_samples - 1; i >= 0; --i) {
            samples[i * 2 + 1] = samples[i]; // Right = Left
            samples[i * 2] = samples[i];     // Left
        }
        br *= 2; // Now stereo size
        printf("Debug: Mono duplicated to stereo, new size: %u\n", br);
    }

    printf("Debug: Filled %u bytes, remaining: %u, position: %u\n", br, player->bytes_remaining, player->current_position);
    return (int)br;
}

// Reconfigure I2S for sample rate (deinit, set AudioFreq, reinit)
static int reconfigure_i2s(I2S_HandleTypeDef *hi2s, uint32_t sample_rate) {
    HAL_I2S_DeInit(hi2s);
    hi2s->Init.AudioFreq = sample_rate;
    if (HAL_I2S_Init(hi2s) != HAL_OK) {
        printf("Debug: I2S reinit failed for %u Hz.\n", sample_rate);
        return WAV_ERR_I2S_CONFIG;
    }
    printf("Debug: I2S reconfigured to %u Hz.\n", sample_rate);
    return WAV_ERR_NONE;
}

int wav_player_init(WAV_Player_t *player, I2S_HandleTypeDef *hi2s) {
    memset(player, 0, sizeof(WAV_Player_t));
    player->hi2s = hi2s;
    player->state = WAV_STOPPED;
    g_player = player;

    // Init SD card via SDFaths
    if (SD_init() != 0) {
        printf("Debug: SD_init failed.\n");
        return WAV_ERR_SD_INIT;
    }

    // Mount FatFS
    FATFS fs;
    FRESULT res = f_mount(&fs, "0:", 1);
    if (res != FR_OK) {
        printf("Debug: FatFS mount failed: %d\n", res);
        return WAV_ERR_FS_MOUNT;
    }
    printf("Debug: WAV player initialized successfully.\n");
    return WAV_ERR_NONE;
}

int wav_player_play(WAV_Player_t *player, const char *filename) {
    if (player->state != WAV_STOPPED) {
        printf("Debug: Player not stopped (state: %d).\n", player->state);
        return WAV_ERR_STATE;
    }

    // Open file
    FRESULT res = f_open(&player->file, filename, FA_READ);
    if (res != FR_OK) {
        printf("Debug: Failed to open %s: %d\n", filename, res);
        return WAV_ERR_FILE_OPEN;
    }
    printf("Debug: Opened file %s, size: %u bytes.\n", filename, (uint32_t)f_size(&player->file));

    // Parse header
    int ret = parse_wav_header(player);
    if (ret != WAV_ERR_NONE) {
        f_close(&player->file);
        return ret;
    }

    // Reconfigure I2S for sample rate
    ret = reconfigure_i2s(player->hi2s, player->header.sample_rate);
    if (ret != WAV_ERR_NONE) {
        f_close(&player->file);
        return ret;
    }

    // Fill initial buffers
    int br1 = fill_buffer(player, player->buffer1, WAV_BUFFER_SIZE);
    int br2 = fill_buffer(player, player->buffer2, WAV_BUFFER_SIZE);
    if (br1 <= 0 || br2 <= 0) {
        f_close(&player->file);
        return WAV_ERR_READ_FAIL;
    }

    // Start DMA (transmit full double buffer as uint16_t)
    uint8_t full_buffer[2 * WAV_BUFFER_SIZE];
    memcpy(full_buffer, player->buffer1, WAV_BUFFER_SIZE);
    memcpy(full_buffer + WAV_BUFFER_SIZE, player->buffer2, WAV_BUFFER_SIZE);
    if (HAL_I2S_Transmit_DMA(player->hi2s, (uint16_t *)full_buffer, (2 * WAV_BUFFER_SIZE) / 2) != HAL_OK) { // Samples count
        printf("Debug: I2S DMA start failed.\n");
        f_close(&player->file);
        return WAV_ERR_I2S_CONFIG;
    }

    player->state = WAV_PLAYING;
    printf("Debug: Playback started.\n");
    return WAV_ERR_NONE;
}

int wav_player_pause(WAV_Player_t *player) {
    if (player->state != WAV_PLAYING) {
        printf("Debug: Not playing (state: %d).\n", player->state);
        return WAV_ERR_STATE;
    }
    HAL_I2S_DMAPause(player->hi2s);
    player->state = WAV_PAUSED;
    printf("Debug: Playback paused at position %u.\n", player->current_position);
    return WAV_ERR_NONE;
}

int wav_player_resume(WAV_Player_t *player) {
    if (player->state != WAV_PAUSED) {
        printf("Debug: Not paused (state: %d).\n", player->state);
        return WAV_ERR_STATE;
    }
    HAL_I2S_DMAResume(player->hi2s);
    player->state = WAV_PLAYING;
    printf("Debug: Playback resumed from position %u.\n", player->current_position);
    return WAV_ERR_NONE;
}

void wav_player_stop(WAV_Player_t *player) {
    if (player->state == WAV_STOPPED) return;

    HAL_I2S_DMAStop(player->hi2s);
    f_close(&player->file);
    player->state = WAV_STOPPED;
    player->bytes_remaining = 0;
    player->current_position = 0;
    printf("Debug: Playback stopped.\n");
}

// Callbacks for double buffering (fill next half)
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
    if (g_player && g_player->state == WAV_PLAYING) {
        int ret = fill_buffer(g_player, g_player->buffer1, WAV_BUFFER_SIZE);
        if (ret <= 0 && g_player->bytes_remaining == 0) {
            wav_player_stop(g_player);
        } else if (ret < 0) {
            printf("Debug: Fill error in half cplt.\n");
            wav_player_stop(g_player);
        }
    }
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
    if (g_player && g_player->state == WAV_PLAYING) {
        int ret = fill_buffer(g_player, g_player->buffer2, WAV_BUFFER_SIZE);
        if (ret <= 0 && g_player->bytes_remaining == 0) {
            wav_player_stop(g_player);
        } else if (ret < 0) {
            printf("Debug: Fill error in cplt.\n");
            wav_player_stop(g_player);
        }
    }
}
