#ifndef WAV_PLAYER_H
#define WAV_PLAYER_H

#include "ff.h"

FRESULT WAV_Player_Start(const char *path);
void WAV_Player_Process(void);
void WAV_Player_Stop(void);
int WAV_Player_IsPlaying(void);
FRESULT WAV_Player_GetResult(void);

#endif