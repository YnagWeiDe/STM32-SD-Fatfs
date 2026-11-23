#ifndef __I2SD_H
#define __I2SD_H

#include "stm32f4xx_hal.h"
#include "ff.h"
#include <stdint.h>
#include <stdio.h>  // for printf

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    I2SD_STATE_IDLE = 0,
    I2SD_STATE_PLAYING,
    I2SD_STATE_STOPPING,
    I2SD_STATE_STOPPED
} I2SD_State;

typedef struct {
    uint32_t sampleRate;     // e.g., 44100
    uint16_t bitsPerSample;  // 16
    uint16_t numChannels;    // 1 or 2
    uint32_t dataOffset;     // data chunk start offset
    uint32_t dataSize;       // data chunk size (in bytes)
} WAV_Info;

typedef struct {
    FIL file;                // FatFs file
    uint8_t fileOpened;      // File open status
    WAV_Info wav;            // WAV metadata
    uint32_t dataRemaining;  // Remaining data bytes

    volatile uint8_t halfEmpty;  // Half buffer needs filling
    volatile uint8_t fullEmpty;  // Full buffer needs filling
    I2SD_State state;        // Current state

    FATFS fs;                // FatFs instance for mounting
} I2SD_HandleTypeDef;

extern I2SD_HandleTypeDef hi2sd;
extern I2S_HandleTypeDef hi2s2;  // CubeMX generated

void I2SD_Init(I2SD_HandleTypeDef *hi2sd);  // Initialize SD + FatFs + state
int I2SD_PlayWav(I2SD_HandleTypeDef *hi2sd, const char *filename);  // Play WAV
void I2SD_RequestStop(I2SD_HandleTypeDef *hi2sd);  // Soft stop
void I2SD_StopNow(I2SD_HandleTypeDef *hi2sd);      // Hard stop

void I2SD_Process(I2SD_HandleTypeDef *hi2sd);  // Main loop / TIM2 callback

void I2SD_OnTxHalfCplt(I2SD_HandleTypeDef *hi2sd);
void I2SD_OnTxCplt(I2SD_HandleTypeDef *hi2sd);

#ifdef __cplusplus
}
#endif

#endif /* __I2SD_H */
