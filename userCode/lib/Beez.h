#ifndef __BEEP_H
#define __BEEP_H

#include "sys.h"
#include "stdlib.h"
#include "delay.h"

// 定义蜂鸣器GPIO宏
#define BEEP_PIN PBout(5)

// 蜂鸣器初始化
void BEEP_Init(void);
// 按频率发声，单位Hz，1000为静音
void Beep_PlayFrequency(uint16_t frequency);
// 按时长发声，单位ms
void Beep_PlayDuration(uint16_t duration_ms);
// 播放成功提示音
void Beep_PlaySuccess(void);
// 播放失败提示音
void Beep_PlayFailure(void);
// 播放乐曲
void Beep_PlayMusic(void);

#endif


