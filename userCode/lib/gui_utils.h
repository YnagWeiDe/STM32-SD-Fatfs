#ifndef __GUI_UTILS_H__
#define __GUI_UTILS_H__

#include "main.h"
#include "ili9341.h"

extern uint16_t Asii0816[96][16];

// 日志显示相关常量定义
//1. 日志每行最多64字符
//2. 日志最多容纳12行
//3. 每次最多显示5个行
#define LOG_LINE_MAX_LEN 64
#define LOG_INDEX_MAX 12
#define SHOW_LEN 5
// 工具函数声明
void GUI_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void GUI_DrawRoundRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                      uint16_t radius, uint16_t color);
void GUI_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void GUI_DrawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                        uint16_t progress, uint16_t color);
void GUI_DrawText(uint16_t x, uint16_t y, const char* text, uint16_t color, uint16_t bg_color);
void GUI_LogPage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const char* log_text, uint16_t text_color, uint16_t bg_color);
void GUI_LogList( uint16_t text_color, uint16_t bg_color);
#endif


