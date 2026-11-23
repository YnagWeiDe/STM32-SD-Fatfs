#include "gui_utils.h"
#include "ili9341.h"
#include "stdio.h"

char log_lines[LOG_INDEX_MAX][LOG_LINE_MAX_LEN];
uint8_t log_line_index = 0;
uint16_t list_x = 0;
uint16_t list_y = 0;
uint16_t list_width = 0;
uint16_t list_height = 0;
uint8_t  list_index=0; 

// 绘制矩形边框
void GUI_DrawRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    // 上边
    LCD_DrawLine(x, y, x + width - 1, y, color);
    // 下边
    LCD_DrawLine(x, y + height - 1, x + width - 1, y + height - 1, color);
    // 左边
    LCD_DrawLine(x, y, x, y + height - 1, color);
    // 右边
    LCD_DrawLine(x + width - 1, y, x + width - 1, y + height - 1, color);
}

// 绘制圆角矩形
void GUI_DrawRoundRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                       uint16_t radius, uint16_t color)
{
    // 上边
    LCD_DrawLine(x + radius, y, x + width - radius - 1, y, color);
    // 下边
    LCD_DrawLine(x + radius, y + height - 1, x + width - radius - 1, y + height - 1, color);
    // 左边
    LCD_DrawLine(x, y + radius, x, y + height - radius - 1, color);
    // 右边
    LCD_DrawLine(x + width - 1, y + radius, x + width - 1, y + height - radius - 1, color);

    // 四个圆角（用四段圆弧近似）
    LCD_DrawFilledCircle(x + radius, y + radius, radius, color);
    LCD_DrawFilledCircle(x + width - radius - 1, y + radius, radius, color);
    LCD_DrawFilledCircle(x + radius, y + height - radius - 1, radius, color);
    LCD_DrawFilledCircle(x + width - radius - 1, y + height - radius - 1, radius, color);
}

// 填充矩形
void GUI_FillRect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    LCD_SetWindow(x, y, width, height);

    for (uint32_t i = 0; i < width * height; i++)
    {
        LCD_WriteData_16Bit(color);
    }
}

// 绘制进度条
void GUI_DrawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                         uint16_t progress, uint16_t color)
{
    // 绘制边框
    GUI_DrawRect(x, y, width, height, WHITE);

    // 计算进度填充宽度
    uint16_t fill_width = (width - 2) * progress / 100;
    if (fill_width > 0)
    {
        // 填充进度区域
        GUI_FillRect(x + 1, y + 1, fill_width, height - 2, color);
    }

    // 如果进度未满，填充剩余部分为背景色
    if (fill_width < width - 2)
    {
        GUI_FillRect(x + 1 + fill_width, y + 1, width - 2 - fill_width, height - 2, BLACK);
    }
}

// 绘制文本背景（用于改善文字可读性）
void GUI_DrawTextBackground(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t bg_color)
{
    GUI_FillRect(x, y, width, height, bg_color);
}

// 绘制分隔线
void GUI_DrawSeparator(uint16_t x, uint16_t y, uint16_t length, uint16_t color)
{
    LCD_DrawLine(x, y, x + length, y, color);
}

/**
 * @brief 在指定位置绘制文本
 * @param x: 起始x坐标
 * @param y: 起始y坐标  
 * @param text: 要显示的文本字符串
 * @param color: 文本颜色
 * @param bg_color: 背景颜色
 */
void GUI_DrawText(uint16_t x, uint16_t y, const char *text, uint16_t color, uint16_t bg_color)
{
    while (*text)
    {
        char ch = *text;
        
        // 字符有效性检查，只显示可打印ASCII字符(32-127)
        if (ch < 32 || ch > 127)
        {
            ch = '?'; // 非法字符显示为问号
        }
        
        // 获取字符在字体数组中的指针(ASCII从32开始，所以减去32)
        const uint16_t *font_ptr = Asii0816[ch - 32];
        
        // 遍历字符的16行像素
        for (uint8_t row = 0; row < 16; row++)
        {
            uint16_t row_data = font_ptr[row]; // 获取当前行的像素数据
            
            // 遍历字符的8列像素
            for (uint8_t col = 0; col < 8; col++)
            {
                // 根据像素位决定显示文本色还是背景色
                uint16_t px = (row_data & (0x80 >> col)) ? color : bg_color;
                
                // 设置单个像素的显示窗口
                LCD_SetWindow(x + col, y + row, 1, 1);
                
                // 写入像素颜色数据
                LCD_WriteData_16Bit(px);
            }
        }
        
        x += 8;  // 移动到下一个字符位置(8像素宽)
        text++;  // 处理下一个字符
    }
}

/**
 * @brief 绘制日志列表
 * @param text_color: 文本颜色
 * @param bg_color: 背景颜色
 */
void GUI_LogList(uint16_t text_color, uint16_t bg_color)
{
    // 绘制列表区域背景(固定为黄色)
    GUI_FillRect(list_x, list_y, list_width, list_height, bg_color);
    
    // 只显示5行，从log_line_index开始
    uint8_t line_start = log_line_index;
    
    for (uint8_t i = 0; i < SHOW_LEN; i++)
    {
        uint8_t idx = line_start + i;
        
        // 检查索引是否在有效范围内
        if (idx < LOG_INDEX_MAX)
        {
            // 在列表区域内均匀排布，每行间隔20像素
            GUI_DrawText(list_x, 
                        list_y + 5 + (i % SHOW_LEN) * 20, 
                        log_lines[idx], 
                        text_color, 
                        bg_color);
        }
    }
}

/**
 * @brief 日志页面绘制函数
 * @param x: 页面起始x坐标
 * @param y: 页面起始y坐标  
 * @param width: 页面宽度
 * @param height: 页面高度
 * @param log_text: 日志文本(参数未使用)
 * @param text_color: 文本颜色
 * @param bg_color: 背景颜色
 */
void GUI_LogPage(uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                const char *log_text, uint16_t text_color, uint16_t bg_color)
{
    uint8_t my_dir = GetLcdDir(); // 获取LCD方向(变量未使用)
    
    // 绘制页面背景 
    GUI_FillRect(x, y, width, height, bg_color);

    // 绘制日志基础信息其中Log Data将与log_text合并显示

    char log_data_text[100];
    sprintf(log_data_text, "Log Data %s", log_text);//拼接字符串
    GUI_DrawText(x + 30, y + 5, log_data_text, text_color, bg_color);
    GUI_DrawText(x + 30, y + 25, "Status: OK", text_color, bg_color);
    GUI_DrawText(x + 30, y + 45, "Time: 2021-01-01 12:00:00", text_color, bg_color);
    
    // 绘制分割线
    GUI_DrawSeparator(x + 10, y + 65, width - 20, WHITE);
    
    // 绘制日志标题
    GUI_DrawText(x + 30, y + 70, "LookForMe Log", text_color, bg_color);

    // 调试信息输出
    printf("x: %d, y: %d, width: %d, height: %d\n", x, y, width, height);
    
    // 设置列表区域参数
    list_x = x + 30;           // 列表起始x坐标
    list_y = y + 90;           // 列表起始y坐标
    list_width = width - 60;   // 列表宽度(两边各留30像素边距)
    list_height = y + 120;     // 列表高度

    // 绘制日志列表
    GUI_LogList(text_color, bg_color);
}
