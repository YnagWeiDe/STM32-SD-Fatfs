#ifndef __IMAGE_DISPLAY_H__
#define __IMAGE_DISPLAY_H__

#include "main.h"
#include "ili9341.h"
#include "bmp_reader.h"
#include "stdio.h"
#include "SDFaths.h"

// 显示选项
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t display_width;
    uint16_t display_height;
    uint8_t  keep_aspect_ratio;
} DisplayOptions;

// 函数声明
uint8_t IMG_DisplayBMP(const char* filename, uint16_t x, uint16_t y);
uint8_t IMG_DisplayBMP_Block(const char* filename, uint16_t x, uint16_t y, uint16_t block_height);
uint8_t IMG_DisplayBMPWithOptions(const char* filename, DisplayOptions* options);
uint8_t IMG_DisplayBMPFromBuffer(uint16_t* buffer, uint32_t width, uint32_t height, 
                                uint16_t x, uint16_t y);
uint8_t BMP_ReadPixelDataPartial(const char* filename, BMP_ImageInfo* info, 
                                uint16_t* line_buffer, uint32_t buffer_size,
                                uint16_t x, uint16_t y);
uint8_t BMP_ReadPixelDataBlock(const char* filename, BMP_ImageInfo* info, 
                              uint16_t* block_buffer, uint32_t buffer_size,
                              uint32_t start_row, uint32_t num_rows,
                              uint16_t x, uint16_t y);
 void ScaleImageNearest(uint16_t* src, uint32_t src_w, uint32_t src_h,
                            uint16_t* dst, uint32_t dst_w, uint32_t dst_h);

#endif
 
 
 
 
 
 