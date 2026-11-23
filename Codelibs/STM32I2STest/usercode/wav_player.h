/* wav_player.h */
/**
 * @file wav_player.h
 * @brief WAV Player Library Header
 * 
 * This library provides functionality to play WAV audio files using I2S on STM32F407VGT6.
 * It supports dynamic parsing of WAV headers (not fixed to 44 bytes), mono/stereo channels,
 * and uses FatFS for SD card file access. The library is designed with low coupling:
 * - SD card operations are abstracted via SDFaths.h functions (e.g., SD_init, SD_read).
 * - I2S operations use HAL library.
 * - Debugging information is printed via printf (redirected to UART if needed).
 * 
 * Improvements in this version:
 * - Dynamic sample rate support: Reconfigures I2S PLL and AudioFreq based on WAV header.
 * - Added pause/resume functionality.
 * - Enhanced error handling with specific return codes.
 * - Better buffer management: Ensures alignment and handles partial fills.
 * - Supports only 16-bit PCM (extendable).
 * 
 * Assumptions:
 * - WAV files are PCM uncompressed (format == 1).
 * - Bit depth: 16-bit.
 * - Sampling rates: 8kHz to 48kHz (common rates; hardware PLL must support).
 * - Uses double buffering for smooth playback via DMA.
 * - I2S configured as Master Transmit, 16-bit data.
 * 
 * Usage:
 * 1. Call wav_player_init() to initialize I2S and FatFS.
 * 2. Call wav_player_play(filename) to start playing a WAV file.
 * 3. Call wav_player_pause() / wav_player_resume() as needed.
 * 4. Call wav_player_stop() to stop playback.
 * 
 * Dependencies:
 * - fatfs.h, SDFaths.h (for SD card access).
 * - i2s.h (for I2S HAL).
 * - stdio.h (for printf debugging).
 * - stdint.h, string.h.
 * 
 * Author: Grok 4 (xAI)
 * Date: November 15, 2025
 * Version: 1.1 (Improved with dynamic sample rate and pause/resume)
 */

#ifndef WAV_PLAYER_H
#define WAV_PLAYER_H

#include "fatfs.h"      // For FatFS file operations
#include "SDFaths.h"    // For SD card init and read (assuming SD_init(), etc.)
#include "i2s.h"        // For I2S HAL
#include <stdio.h>      // For printf debugging
#include <stdint.h>     // For fixed-width types
#include <string.h>     // For memcpy, memset

// Define buffer size (4096 bytes: balanced for DMA and SD read speed)
#define WAV_BUFFER_SIZE 4096

// Error codes
#define WAV_ERR_NONE            0
#define WAV_ERR_SD_INIT        -1
#define WAV_ERR_FS_MOUNT       -2
#define WAV_ERR_FILE_OPEN      -3
#define WAV_ERR_INVALID_HEADER -4
#define WAV_ERR_UNSUPPORTED    -5
#define WAV_ERR_READ_FAIL      -6
#define WAV_ERR_I2S_CONFIG     -7
#define WAV_ERR_STATE          -8

// WAV format structure (parsed from header)
typedef struct {
    uint32_t chunk_id;          // 'RIFF'
    uint32_t chunk_size;
    uint32_t format;            // 'WAVE'
    uint32_t subchunk1_id;      // 'fmt '
    uint32_t subchunk1_size;
    uint16_t audio_format;      // 1 for PCM
    uint16_t num_channels;      // 1 or 2
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t subchunk2_id;      // 'data'
    uint32_t subchunk2_size;    // Data size
    uint32_t data_offset;       // Offset to audio data
} WAV_Header_t;

// Player state
typedef enum {
    WAV_STOPPED,
    WAV_PLAYING,
    WAV_PAUSED
} WAV_State_t;

// WAV Player handle
typedef struct {
    FIL file;                   // FatFS file object
    WAV_Header_t header;        // Parsed WAV header
    I2S_HandleTypeDef *hi2s;    // I2S handle (e.g., &hi2s2)
    uint8_t buffer1[WAV_BUFFER_SIZE]; // Double buffer 1
    uint8_t buffer2[WAV_BUFFER_SIZE]; // Double buffer 2
    uint32_t bytes_remaining;   // Bytes left in file data chunk
    uint32_t current_position;  // Current file position in data chunk (for pause/resume)
    WAV_State_t state;          // Current state
    uint8_t current_buf_idx;    // 0: buffer1, 1: buffer2 (for DMA switching)
} WAV_Player_t;

/**
 * @brief Initialize the WAV player library.
 *        - Initializes SD card via SD_init().
 *        - Mounts FatFS.
 *        - Stores I2S handle (I2S init done externally via MX_I2S_Init).
 * @param player Pointer to WAV_Player_t structure.
 * @param hi2s I2S handle (e.g., &hi2s2).
 * @return WAV_ERR_NONE on success, error code on failure.
 */
int wav_player_init(WAV_Player_t *player, I2S_HandleTypeDef *hi2s);

/**
 * @brief Play a WAV file from SD card.
 *        - Parses header dynamically (handles variable chunk sizes, skips extra chunks).
 *        - Configures I2S dynamically based on header (sample rate, channels, bit depth).
 *        - Starts DMA playback with double buffering.
 * @param player Pointer to WAV_Player_t structure.
 * @param filename Path to WAV file (e.g., "0:/music.wav").
 * @return WAV_ERR_NONE on success, error code on failure.
 */
int wav_player_play(WAV_Player_t *player, const char *filename);

/**
 * @brief Pause playback (stops DMA but keeps file open and position).
 * @param player Pointer to WAV_Player_t structure.
 * @return WAV_ERR_NONE on success, WAV_ERR_STATE if not playing.
 */
int wav_player_pause(WAV_Player_t *player);

/**
 * @brief Resume playback from paused position.
 * @param player Pointer to WAV_Player_t structure.
 * @return WAV_ERR_NONE on success, WAV_ERR_STATE if not paused.
 */
int wav_player_resume(WAV_Player_t *player);

/**
 * @brief Stop playback and close file.
 * @param player Pointer to WAV_Player_t structure.
 */
void wav_player_stop(WAV_Player_t *player);

/**
 * @brief HAL I2S Tx Half Complete Callback (for double buffering).
 *        - Fills the first half of the buffer.
 */
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s);

/**
 * @brief HAL I2S Tx Complete Callback (for double buffering).
 *        - Fills the second half of the buffer.
 */
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s);

#endif // WAV_PLAYER_H
