#ifndef __BMP_READER_H__
#define __BMP_READER_H__

#include "main.h"
#include "ff.h"

// BMP文件头结构
#pragma pack(push, 1)
typedef struct {
    uint16_t signature;      // "BM"
    uint32_t file_size;      // 文件大小
    uint16_t reserved1;      // 保留
    uint16_t reserved2;      // 保留
    uint32_t data_offset;    // 像素数据偏移量
} BMP_FileHeader;

// BMP信息头结构
typedef struct {
    uint32_t header_size;    // 信息头大小
    int32_t  width;          // 图像宽度
    int32_t  height;         // 图像高度
    uint16_t planes;         // 颜色平面数
    uint16_t bits_per_pixel; // 每像素位数
    uint32_t compression;    // 压缩方式
    uint32_t image_size;     // 图像数据大小
    int32_t  x_pixels_per_meter;
    int32_t  y_pixels_per_meter;
    uint32_t colors_used;    // 使用的颜色数
    uint32_t colors_important;
} BMP_InfoHeader;
#pragma pack(pop)

// BMP图片信息结构
typedef struct {
    uint32_t width;
    uint32_t height;
    uint16_t bits_per_pixel;
    uint32_t data_offset;
    uint32_t row_size;
    uint8_t  has_palette;
    uint32_t palette_size;
} BMP_ImageInfo;

// 函数声明
uint8_t BMP_GetInfo(const char* filename, BMP_ImageInfo* info);
uint8_t BMP_ReadPixelData(const char* filename, BMP_ImageInfo* info, 
                         uint16_t* buffer, uint32_t buffer_size);
 void ConvertPixelData(uint8_t* src, uint16_t* dest, uint32_t width, uint16_t bpp);

#endif


