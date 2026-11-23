#include "I2SD.h"
#include <string.h>
#include <stdio.h>
#include "fatfs.h"

extern I2S_HandleTypeDef hi2s2;
extern FATFS USERFatFS; // Global FatFS instance


extern uint8_t SD_init(void);

// DMA Buffer: 2 half-buffers for I2S transmission
#define AUDIO_BUF_SAMPLES 4096  // Number of samples per half-buffer (16-bit samples)
static uint16_t i2sTxBuf[2 * AUDIO_BUF_SAMPLES]; // Double buffering

static int I2SD_ParseWavHeader(FIL *file, WAV_Info *info);
static int I2SD_FillHalfBuffer(I2SD_HandleTypeDef *hi2sd, uint16_t *dst, uint32_t samples);

void I2SD_Init(I2SD_HandleTypeDef *h) {
    memset(h, 0, sizeof(I2SD_HandleTypeDef));
    h->state = I2SD_STATE_IDLE;
}

int I2SD_PlayWav(I2SD_HandleTypeDef *h, const char *filename) {
    FRESULT res;
    if (!h) return -1;

    if (h->state == I2SD_STATE_PLAYING) {
        I2SD_StopNow(h);
    }

    if (SD_init() != 0) {
        printf("SD init failed\r\n");
        return -1;
    }

    res = f_mount(&USERFatFS, "0:", 1);
    if (res != FR_OK) {
        printf("f_mount failed: %d\r\n", res);
        return -1;
    }

    res = f_open(&h->file, "ZARD.wav", FA_READ);
    if (res != FR_OK) {
        printf("f_open failed: %d\r\n", res);
        f_mount(NULL, "0:", 1);
        return -1;
    }
    h->fileOpened = 1;

    if (I2SD_ParseWavHeader(&h->file, &h->wav) != 0) {
        printf("WAV header not supported\r\n");
        f_close(&h->file);
        h->fileOpened = 0;
        f_mount(NULL, "0:", 1);
        return -1;
    }

    printf("WAV: %lu Hz, %u bits, %u ch, dataSize=%lu\r\n",
           h->wav.sampleRate, h->wav.bitsPerSample, h->wav.numChannels, h->wav.dataSize);
		printf("OK1");
    h->dataRemaining = h->wav.dataSize;
    h->halfEmpty = 0;
    h->fullEmpty = 0;
    h->state = I2SD_STATE_PLAYING;

    // Fill the first half and second half of the buffer
    if (I2SD_FillHalfBuffer(h, &i2sTxBuf[0], AUDIO_BUF_SAMPLES) != 0) {
        printf("No audio data (first half)\r\n");
        I2SD_StopNow(h);
        f_close(&h->file);
        h->fileOpened = 0;
        f_mount(NULL, "0:", 1);
        return -1;
    }
		printf("OK2");

    if (I2SD_FillHalfBuffer(h, &i2sTxBuf[AUDIO_BUF_SAMPLES], AUDIO_BUF_SAMPLES) != 0) {
        printf("Second half buffer partially filled\r\n");
    }
		printf("OK3");

    // Start I2S DMA (length = 2 * AUDIO_BUF_SAMPLES, i.e., full buffer size)
    if (HAL_I2S_Transmit_DMA(&hi2s2, i2sTxBuf, AUDIO_BUF_SAMPLES * 2) != HAL_OK) {
        printf("HAL_I2S_Transmit_DMA failed\r\n");
        I2SD_StopNow(h);
        f_close(&h->file);
        h->fileOpened = 0;
        f_mount(NULL, "0:", 1);
        return -1;
    }
		printf("OK4");

    printf("Start playing: %s\r\n", filename);
    return 0;
}

void I2SD_RequestStop(I2SD_HandleTypeDef *h) {
    if (!h) return;
    if (h->state == I2SD_STATE_PLAYING) {
        h->state = I2SD_STATE_STOPPING;
    }
}

void I2SD_StopNow(I2SD_HandleTypeDef *h) {
    if (!h) return;

    HAL_I2S_DMAStop(&hi2s2);

    if (h->fileOpened) {
        f_close(&h->file);
        h->fileOpened = 0;
    }

    f_mount(NULL, "0:", 1);

    h->state = I2SD_STATE_STOPPED;
    h->halfEmpty = 0;
    h->fullEmpty = 0;

    printf("Stop audio\r\n");
}

void I2SD_Process(I2SD_HandleTypeDef *h) {
    if (!h) return;

    if (h->state != I2SD_STATE_PLAYING && h->state != I2SD_STATE_STOPPING) {
        return;
    }

    if (h->halfEmpty) {
        h->halfEmpty = 0;
        if (h->state == I2SD_STATE_PLAYING) {
            if (I2SD_FillHalfBuffer(h, &i2sTxBuf[0], AUDIO_BUF_SAMPLES) != 0) {
                h->state = I2SD_STATE_STOPPING;
            }
        } else {
            memset(&i2sTxBuf[0], 0, AUDIO_BUF_SAMPLES * sizeof(uint16_t));
        }
    }

    if (h->fullEmpty) {
        h->fullEmpty = 0;
        if (h->state == I2SD_STATE_PLAYING) {
            if (I2SD_FillHalfBuffer(h, &i2sTxBuf[AUDIO_BUF_SAMPLES], AUDIO_BUF_SAMPLES) != 0) {
                h->state = I2SD_STATE_STOPPING;
            }
        } else {
            memset(&i2sTxBuf[AUDIO_BUF_SAMPLES], 0, AUDIO_BUF_SAMPLES * sizeof(uint16_t));
        }
    }

    if (h->state == I2SD_STATE_STOPPING && h->halfEmpty == 0 && h->fullEmpty == 0) {
        I2SD_StopNow(h);
    }
}

void I2SD_OnTxHalfCplt(I2SD_HandleTypeDef *h) {
    if (!h) return;
    h->halfEmpty = 1;
}

void I2SD_OnTxCplt(I2SD_HandleTypeDef *h) {
    if (!h) return;
    h->fullEmpty = 1;
}

static int I2SD_ParseWavHeader(FIL *file, WAV_Info *info) {
    UINT br;
    uint8_t buf[12];
    f_lseek(file, 0);

    if (f_read(file, buf, 12, &br) != FR_OK || br != 12) return -1;
    if (memcmp(buf, "RIFF", 4) != 0) return -1;
    if (memcmp(buf + 8, "WAVE", 4) != 0) return -1;

    uint8_t chunkId[4];
    uint32_t chunkSize;

    int haveFmt = 0;
    int haveData = 0;

    while (1) {
        if (f_read(file, chunkId, 4, &br) != FR_OK || br != 4) break;
        if (f_read(file, buf, 4, &br) != FR_OK || br != 4) break;
        chunkSize = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
        uint32_t chunkDataPos = f_tell(file);

        if (memcmp(chunkId, "fmt ", 4) == 0) {
            uint8_t fmt[16];
            if (chunkSize < 16) return -1;
            if (f_read(file, fmt, 16, &br) != FR_OK || br != 16) return -1;

            uint16_t audioFormat = fmt[0] | (fmt[1] << 8);
            uint16_t numChannels = fmt[2] | (fmt[3] << 8);
            uint32_t sampleRate = fmt[4] | (fmt[5] << 8) |
                                  (fmt[6] << 16) | (fmt[7] << 24);
            uint16_t bitsPerSample = fmt[14] | (fmt[15] << 8);

            if (audioFormat != 1) return -1;
            if (bitsPerSample != 16) return -1;
            if (numChannels != 1 && numChannels != 2) return -1;

            info->sampleRate = sampleRate;
            info->bitsPerSample = bitsPerSample;
            info->numChannels = numChannels;

            haveFmt = 1;

            if (chunkSize > 16) {
                f_lseek(file, chunkDataPos + chunkSize);
            }
        } else if (memcmp(chunkId, "data", 4) == 0) {
            info->dataOffset = f_tell(file);
            info->dataSize = chunkSize;
            haveData = 1;
            break;
        } else {
            f_lseek(file, chunkDataPos + chunkSize);
        }
    }

    if (!haveFmt || !haveData) return -1;
    return 0;
}

static int I2SD_FillHalfBuffer(I2SD_HandleTypeDef *h, uint16_t *dst, uint32_t samples) {
    if (!h || !h->fileOpened) return -1;
    UINT br = 0;
    UINT needBytes;
    FRESULT res;
		printf(" T1");
    if (h->wav.numChannels == 2) {
			printf(" T2");
        needBytes = samples * 2;
        if (h->dataRemaining < needBytes) {
					printf("T9");
					needBytes = h->dataRemaining;
					printf("T8");
				}
				printf("T11");

        res = f_read(&h->file, dst, needBytes, &br);
        printf("T12");
				h->dataRemaining -= br;
        if (res != FR_OK || br == 0) {
					printf("T10");
            memset(dst, 0, samples * 2);
					printf("T5");
            return -1;
        }
        if (br < samples * 2) {
            uint32_t validSamples = br / 2;
            memset(&dst[validSamples], 0, (samples - validSamples) * 2);
					printf("T6");
            return -1;
        }
				printf("T4");
        return 0;
    } else if (h->wav.numChannels == 1) {
			printf(" T3");
				uint32_t needBytes = samples * 2;  // stereo 输出，mono 读 samples 字节（每个 sample 2 字节）
				if (h->dataRemaining < needBytes) needBytes = h->dataRemaining;
				uint16_t monoBuf[AUDIO_BUF_SAMPLES];
				res = f_read(&h->file, (uint8_t*)monoBuf, needBytes, &br);  // 读字节
				h->dataRemaining -= br;
				uint32_t validBytes = br;
				uint32_t validSamples = validBytes / 2;  // 实际 samples
				for (uint32_t i = 0; i < validSamples; i++) {
						dst[2 * i] = monoBuf[i];     // L = mono
						dst[2 * i + 1] = monoBuf[i]; // R = mono
				}
				uint32_t remainingSamples = samples - validSamples;  // 注意：samples 是 stereo pairs
				if (remainingSamples > 0) {
						memset(&dst[2 * validSamples], 0, remainingSamples * 4);  // 清零剩余 stereo pairs（每个 4 字节）
				}
				if (res != FR_OK || br == 0 || validSamples < samples / 2) return -1;  // 调整阈值
				return 0;
    } else {
        memset(dst, 0, samples * 2);
        return -1;
    }
}
