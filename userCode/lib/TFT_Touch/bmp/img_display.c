#include "img_display.h"
#include "ff.h"
#include "stdlib.h"

uint8_t IMG_DisplayBMP(const char* filename, uint16_t x, uint16_t y)
{
    FATFS fs;
    FRESULT res;
    BMP_ImageInfo info;
    uint16_t* line_buffer = NULL;
    uint32_t buffer_size;
    uint8_t result = 0;
    
    printf("尝试显示图片: %s\r\n", filename);
    
    // 初始化 SD 卡
    if (SD_init() != 0) {
        printf("SD卡初始化失败\r\n");
        return 0;
    }
    
    // 挂载文件系统
    res = f_mount(&fs, "0:", 1);
    if (res != FR_OK) {
        printf("文件系统挂载失败，错误代码: %d\r\n", res);
        return 0;
    }
    
    // 检查文件是否存在
    FILINFO fno;
    res = f_stat(filename, &fno);
    if (res != FR_OK) {
        printf("文件不存在或无法访问，错误: %d\r\n", res);
        f_mount(NULL, "0:", 1);
        return 0;
    }
    printf("文件存在，大小: %lu 字节\r\n", fno.fsize);
    
    // 获取 BMP 信息
    if (!BMP_GetInfo(filename, &info)) {
        printf("获取BMP信息失败\r\n");
        f_mount(NULL, "0:", 1);
        return 0;
    }
    
    printf("BMP信息: %dx%d, %dbpp, 数据偏移: %lu\r\n", 
           info.width, info.height, info.bits_per_pixel, info.data_offset);
    
    // 检查支持的格式
    if (info.bits_per_pixel != 16 && info.bits_per_pixel != 24) {
        printf("不支持的位深度: %d, 只支持16/24bpp\r\n", info.bits_per_pixel);
        f_mount(NULL, "0:", 1);
        return 0;
    }
    
    // 分配行缓冲区（每次只处理一行）
    buffer_size = info.width;
    printf("分配行缓冲区大小: %lu 像素 (%lu 字节)\r\n", 
           buffer_size, buffer_size * sizeof(uint16_t));
    
    line_buffer = (uint16_t*)malloc(buffer_size * sizeof(uint16_t));
    if (!line_buffer) {
        printf("内存分配失败\r\n");
        f_mount(NULL, "0:", 1);
        return 0;
    }
    
    printf("开始分段读取和显示像素数据...\r\n");
    
    // 使用分段读取函数
    if (BMP_ReadPixelDataPartial(filename, &info, line_buffer, buffer_size, x, y)) {
        printf("图片显示完成\r\n");
        result = 1;
    } else {
        printf("图片显示失败\r\n");
    }
    
    free(line_buffer);
    f_mount(NULL, "0:", 1);
    
    return result;
}

// 优化版：分段读取和显示BMP数据（多行传输）
uint8_t BMP_ReadPixelDataPartial(const char* filename, BMP_ImageInfo* info, 
                                uint16_t* line_buffer, uint32_t buffer_size,
                                uint16_t x, uint16_t y)
{
    FIL file;
    FRESULT res;
    UINT bytes_read;
    
    // 计算每次能处理的行数（至少1行）
    uint32_t pixels_per_line = info->width;
    uint32_t max_lines = buffer_size / pixels_per_line;
    
    if (max_lines == 0) {
        printf("BMP: 缓冲区太小，无法容纳一行数据\r\n");
        return 0;
    }
    
    // 限制最大行数，避免内存碎片
    if (max_lines > 20) {
        max_lines = 20;
    }
    
    printf("BMP: 缓冲区可容纳 %lu 行，每次处理 %lu 行\r\n", 
           buffer_size / pixels_per_line, max_lines);
    
    // 打开文件
    res = f_open(&file, filename, FA_READ);
    if (res != FR_OK) {
        printf("BMP: 无法打开文件\r\n");
        return 0;
    }
    
    // 定位到像素数据
    res = f_lseek(&file, info->data_offset);
    if (res != FR_OK) {
        printf("BMP: 无法定位到像素数据\r\n");
        f_close(&file);
        return 0;
    }
    
    // 分配源数据缓冲区（多行原始数据）
    uint32_t src_buffer_size = info->row_size * max_lines;
    uint8_t* src_buffer = (uint8_t*)malloc(src_buffer_size);
    if (!src_buffer) {
        printf("BMP: 无法分配源数据缓冲区\r\n");
        f_close(&file);
        return 0;
    }
    
    printf("开始分段读取和显示...\r\n");
    
    uint32_t processed_rows = 0;
    
    // 分段处理：每次处理多行
    while (processed_rows < info->height) {
        // 计算本次要处理的行数
        uint32_t current_lines = info->height - processed_rows;
        if (current_lines > max_lines) {
            current_lines = max_lines;
        }
        
        // 读取多行原始数据
        uint32_t bytes_to_read = info->row_size * current_lines;

        res = f_read(&file, src_buffer, bytes_to_read, &bytes_read);
        if (res != FR_OK || bytes_read != bytes_to_read) {
            printf("BMP: 第%lu行起读取失败\r\n", processed_rows);
            free(src_buffer);
            f_close(&file);
            return 0;
        }
        
        // 处理每一行数据
        for (uint32_t batch_row = 0; batch_row < current_lines; batch_row++) {
            uint8_t* src_row_start = src_buffer + (batch_row * info->row_size);
            uint16_t* dest_row_start = line_buffer + (batch_row * info->width);
            
            // 转换像素格式
            ConvertPixelData(src_row_start, dest_row_start, info->width, info->bits_per_pixel);
            
            // 交换字节序（如果需要）
            for (uint32_t i = 0; i < info->width; i++) {
                dest_row_start[i] = (dest_row_start[i] >> 8) | (dest_row_start[i] << 8);
            }
        }
        
        // 设置多行显示窗口
        LCD_SetWindow(x, y + processed_rows, info->width, current_lines);
        
        // 批量写入多行数据
        LCD_WriteLine_DMA(line_buffer, info->width * current_lines);
        
        processed_rows += current_lines;
        
        // 显示进度
        if (processed_rows % 50 == 0 || processed_rows == info->height) {
            printf("已显示: %lu/%lu 行 (%.1f%%)\r\n", 
                   processed_rows, info->height, processed_rows * 100.0f / info->height);
        }
    }
    
    printf("图片显示完成: %lux%lu\r\n", info->width, info->height);
    
    free(src_buffer);
    f_close(&file);
    return 1;
}

// 新增：分块读取和显示（更节省内存）
uint8_t IMG_DisplayBMP_Block(const char* filename, uint16_t x, uint16_t y, uint16_t block_height)
{
    FATFS fs;
    FRESULT res;
    BMP_ImageInfo info;
    uint16_t* block_buffer = NULL;
    uint32_t buffer_size;
    uint8_t result = 0;
    
    printf("分块显示图片: %s, 块高度: %d\r\n", filename, block_height);
    
    // 初始化 SD 卡和挂载文件系统（与IMG_DisplayBMP相同）
    if (SD_init() != 0) {
        printf("SD卡初始化失败\r\n");
        return 0;
    }
    
    res = f_mount(&fs, "0:", 1);
    if (res != FR_OK) {
        printf("文件系统挂载失败: %d\r\n", res);
        return 0;
    }
    
    // 获取 BMP 信息
    if (!BMP_GetInfo(filename, &info)) {
        printf("获取BMP信息失败\r\n");
        f_mount(NULL, "0:", 1);
        return 0;
    }
    
    // 分配块缓冲区
    buffer_size = info.width * block_height;
    printf("分配块缓冲区: %lux%d = %lu 像素\r\n", info.width, block_height, buffer_size);
    
    block_buffer = (uint16_t*)malloc(buffer_size * sizeof(uint16_t));
    if (!block_buffer) {
        printf("块缓冲区分配失败\r\n");
        f_mount(NULL, "0:", 1);
        return 0;
    }
    
    // 分块处理
    uint32_t remaining_height = info.height;
    uint32_t current_y = 0;
    uint32_t block_count = 0;
    
    while (remaining_height > 0) {
        uint16_t current_block_height = (remaining_height > block_height) ? block_height : remaining_height;
        
        printf("处理块 %lu: Y=%lu到%lu, 高度=%d\r\n", 
               ++block_count, current_y, current_y + current_block_height - 1, current_block_height);
        
        // 读取并显示当前块
        if (BMP_ReadPixelDataBlock(filename, &info, block_buffer, buffer_size, 
                                  current_y, current_block_height, x, y + current_y)) {
            current_y += current_block_height;
            remaining_height -= current_block_height;
        } else {
            printf("块 %lu 处理失败\r\n", block_count);
            break;
        }
    }
    
    if (remaining_height == 0) {
        printf("全部分块处理完成\r\n");
        result = 1;
    }
    
    free(block_buffer);
    f_mount(NULL, "0:", 1);
    return result;
}

// 新增：读取指定区域的数据块
uint8_t BMP_ReadPixelDataBlock(const char* filename, BMP_ImageInfo* info, 
                              uint16_t* block_buffer, uint32_t buffer_size,
                              uint32_t start_row, uint32_t num_rows,
                              uint16_t x, uint16_t y)
{
    FIL file;
    FRESULT res;
    UINT bytes_read;
    
    // 检查缓冲区大小
    uint32_t required_size = info->width * num_rows;
    if (buffer_size < required_size) {
        printf("BMP: 块缓冲区太小: %lu < %lu\r\n", buffer_size, required_size);
        return 0;
    }
    
    // 打开文件
    res = f_open(&file, filename, FA_READ);
    if (res != FR_OK) {
        return 0;
    }
    
    // 定位到起始行
    uint32_t start_offset = info->data_offset + (info->height - 1 - start_row) * info->row_size;
    res = f_lseek(&file, start_offset);
    if (res != FR_OK) {
        f_close(&file);
        return 0;
    }
    
    uint8_t* src_row_buffer = (uint8_t*)malloc(info->row_size);
    if (!src_row_buffer) {
        f_close(&file);
        return 0;
    }
    
    // 读取指定行数的数据
    uint16_t* current_buffer_pos = block_buffer;
    
    for (uint32_t i = 0; i < num_rows; i++) {
        res = f_read(&file, src_row_buffer, info->row_size, &bytes_read);
        if (res != FR_OK || bytes_read != info->row_size) {
            free(src_row_buffer);
            f_close(&file);
            return 0;
        }
        
        // 转换像素格式
        ConvertPixelData(src_row_buffer, current_buffer_pos, info->width, info->bits_per_pixel);
        current_buffer_pos += info->width;
        
        // 移动到上一行（BMP是倒序存储的）
        if (i < num_rows - 1) {
            f_lseek(&file, f_tell(&file) - 2 * info->row_size);
        }
    }
    
    // 设置显示窗口并写入数据
    LCD_SetWindow(x, y, info->width, num_rows);
    
    LCD_WriteLine_DMA(block_buffer, required_size);
    
    free(src_row_buffer);
    f_close(&file);
    return 1;
}
// 简单的图像缩放函数（最近邻插值）
 void ScaleImageNearest(uint16_t* src, uint32_t src_w, uint32_t src_h,
                            uint16_t* dst, uint32_t dst_w, uint32_t dst_h)
{
    float x_ratio = (float)src_w / dst_w;
    float y_ratio = (float)src_h / dst_h;
    
    for (uint32_t y = 0; y < dst_h; y++) {
        for (uint32_t x = 0; x < dst_w; x++) {
            uint32_t src_x = (uint32_t)(x * x_ratio);
            uint32_t src_y = (uint32_t)(y * y_ratio);
            dst[y * dst_w + x] = src[src_y * src_w + src_x];
        }
    }
}
uint8_t IMG_DisplayBMPWithOptions(const char* filename, DisplayOptions* options)
{
    BMP_ImageInfo info;
    uint16_t* src_buffer;
    uint16_t* dst_buffer = NULL;
    uint32_t src_size;
    uint8_t result = 0;
    
    if (!BMP_GetInfo(filename, &info)) {
        return 0;
    }
    
    src_size = info.width * info.height;
    src_buffer = (uint16_t*)malloc(src_size * sizeof(uint16_t));
    if (!src_buffer) {
        return 0;
    }
    
    if (BMP_ReadPixelData(filename, &info, src_buffer, src_size)) {
        // 如果需要缩放
        if (options->display_width != info.width || options->display_height != info.height) {
            dst_buffer = (uint16_t*)malloc(options->display_width * options->display_height * sizeof(uint16_t));
            if (dst_buffer) {
                // 缩放图像（简单的最近邻插值）
                ScaleImageNearest(src_buffer, info.width, info.height,
                                dst_buffer, options->display_width, options->display_height);
                
                LCD_SetWindow(options->x, options->y, 
                            options->display_width, options->display_height);
                
                // 批量写入数据
                LCD_WriteLine_DMA(dst_buffer, options->display_width * options->display_height);
                
                free(dst_buffer);
                result = 1;
            }
        } else {
            // 直接显示原图
            IMG_DisplayBMPFromBuffer(src_buffer, info.width, info.height, 
                                   options->x, options->y);
            result = 1;
        }
    }
    
    free(src_buffer);
    return result;
}

uint8_t IMG_DisplayBMPFromBuffer(uint16_t* buffer, uint32_t width, uint32_t height, 
                                uint16_t x, uint16_t y)
{
    // 设置显示窗口
	
    LCD_SetWindow(x, y, width, height);
    
    // 批量写入像素数据
    LCD_WriteLine_DMA(buffer, width * height);
    
    return 1;
}



