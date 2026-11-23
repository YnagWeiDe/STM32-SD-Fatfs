// wav_parser.h
#ifndef WAV_PARSER_H
#define WAV_PARSER_H

#include <stdint.h>
#include "ff.h"      // FatFs文件系统
#include "sd.h"      // SD卡驱�?
#include "stdio.h"   // 标准输入输出
#include "string.h"

extern volatile uint32_t DApc;
extern volatile uint8_t DACdone;
extern uint8_t wav_buf[32768];
extern uint32_t wav_SampleRate;
extern uint16_t wav_CHnum;
extern uint16_t wav_SampleBits;
extern uint32_t wav_DataLength;

// WAV文件头结构体
#pragma pack(push, 1) // 确保无字节对�?
typedef struct {
    // RIFF区块
    char chunkID[4];        // "RIFF"
    uint32_t chunkSize;     // 文件总大�?-8
    char format[4];         // "WAVE"
    
    // fmt区块
    char subchunk1ID[4];    // "fmt "
    uint32_t subchunk1Size; // fmt区块大小 (16 for PCM)
    uint16_t audioFormat;   // 1 = PCM
    uint16_t numChannels;   // 声道�?
    uint32_t sampleRate;    // 采样�? (Hz)
    uint32_t byteRate;      // 每秒字节�?
    uint16_t blockAlign;    // 块对�? (每个样本的字节数)
    uint16_t bitsPerSample; // 位深�? (8, 16, 24, 32)
    
    // data区块�?
    char subchunk2ID[4];    // "data"
    uint32_t subchunk2Size; // 音频数据大小
} WAV_Header;
#pragma pack(pop) // 恢复默认对齐

// 音频信息结构
typedef struct {
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint16_t bitsPerSample;
    uint32_t dataSize;
    uint32_t dataOffset;  
    uint32_t duration;
	
} AudioInfo;

// 函数声明
int Parse_WAV_File(const char* filename, AudioInfo* info);
int Read_WAV_AudioData(const char* filename, void* buffer, uint32_t offset, uint32_t size);
void Print_AudioInfo(AudioInfo* info);
int Find_Chunk(FIL* file, const char* chunkId, uint32_t* offset, uint32_t* size);
int Read_WAV_ToBuffer(AudioInfo* info, void* buffer, uint32_t offset, uint32_t size);

#endif // WAV_PARSER_H
