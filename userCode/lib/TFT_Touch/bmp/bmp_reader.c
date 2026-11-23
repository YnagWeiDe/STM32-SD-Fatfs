#include "bmp_reader.h"
#include "ff.h"
#include "stdio.h"

uint8_t BMP_GetInfo(const char* filename, BMP_ImageInfo* info)
{
    FIL file;
    FRESULT res;
    BMP_FileHeader file_header;
    BMP_InfoHeader info_header;
    UINT bytes_read;
    
    // 打开文件
    res = f_open(&file, filename, FA_READ);
    if (res != FR_OK) {
        printf("BMP: Failed to open file %s, error: %d\r\n", filename, res);
        return 0;
    }
    
    // 读取文件头
    res = f_read(&file, &file_header, sizeof(BMP_FileHeader), &bytes_read);
    if (res != FR_OK || bytes_read != sizeof(BMP_FileHeader)) {
        f_close(&file);
        return 0;
    }
    
    // 检查BMP签名
    if (file_header.signature != 0x4D42) { // "BM"
        printf("BMP: Invalid signature: 0x%04X\r\n", file_header.signature);
        f_close(&file);
        return 0;
    }
    
    // 读取信息头
    res = f_read(&file, &info_header, sizeof(BMP_InfoHeader), &bytes_read);
    if (res != FR_OK || bytes_read != sizeof(BMP_InfoHeader)) {
        f_close(&file);
        return 0;
    }
    
    // 填充信息结构
    info->width = (info_header.width > 0) ? info_header.width : -info_header.width;
    info->height = (info_header.height > 0) ? info_header.height : -info_header.height;
    info->bits_per_pixel = info_header.bits_per_pixel;
    info->data_offset = file_header.data_offset;
    
    // 计算每行字节数（4字节对齐）
    info->row_size = ((info->width * info->bits_per_pixel + 31) / 32) * 4;
    
    // 检查是否有调色板
    info->has_palette = (info->bits_per_pixel <= 8);
    info->palette_size = (info->has_palette) ? 
                        (1 << info->bits_per_pixel) * 4 : 0;
    
    f_close(&file);
    return 1;
}


// 像素数据转换函数
 void ConvertPixelData(uint8_t* src, uint16_t* dest, uint32_t width, uint16_t bpp)
{
    switch (bpp) {
        case 24: // RGB888 -> RGB565
            for (uint32_t i = 0; i < width; i++) {
                uint8_t b = src[i * 3];
                uint8_t g = src[i * 3 + 1];
                uint8_t r = src[i * 3 + 2];
                dest[i] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
            }
            break;
            
        case 16: // RGB565 (直接使用)
            for (uint32_t i = 0; i < width; i++) {
                dest[i] = ((uint16_t*)src)[i];
            }
            break;
            
        // 可以添加其他位深度的支持
        default:
            printf("BMP: Unsupported bit depth: %d\r\n", bpp);
            break;
    }
}
//获取图像数据
uint8_t BMP_ReadPixelData(const char* filename, BMP_ImageInfo* info, 
                         uint16_t* buffer, uint32_t buffer_size)
{
    FIL file;
    FRESULT res;
    UINT bytes_read;
    uint32_t required_size;
    
    // 检查缓冲区大小
    required_size = info->width * info->height;
    if (buffer_size < required_size) {
        printf("BMP: Buffer too small: %lu < %lu\r\n", buffer_size, required_size);
        return 0;
    }
    
    // 打开文件
    res = f_open(&file, filename, FA_READ);
    if (res != FR_OK) {
        return 0;
    }
    
    // 定位到像素数据
    res = f_lseek(&file, info->data_offset);
    if (res != FR_OK) {
        f_close(&file);
        return 0;
    }
    
    uint8_t* row_buffer = (uint8_t*)malloc(info->row_size);
    if (!row_buffer) {
        f_close(&file);
        return 0;
    }
    
    // 读取像素数据并转换
    for (uint32_t y = 0; y < info->height; y++) {
        res = f_read(&file, row_buffer, info->row_size, &bytes_read);
        if (res != FR_OK || bytes_read != info->row_size) {
            free(row_buffer);
            f_close(&file);
            return 0;
        }
        
        // 转换像素格式
        uint32_t row_start = (info->height - 1 - y) * info->width; // BMP是倒着存储的
        ConvertPixelData(row_buffer, &buffer[row_start], info->width, info->bits_per_pixel);
    }
    
    free(row_buffer);
    f_close(&file);
    return 1;
}

