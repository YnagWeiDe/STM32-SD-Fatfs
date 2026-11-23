#include "ili9341.h"
#include <stdlib.h>  // 用于 malloc/free

_lcd_dev lcddev;

// 声明SPI句柄（在main.c中定义）
extern SPI_HandleTypeDef LCD_SPI_HANDLE;

// ==================== 阻塞传输模式函数 ====================

/**
 * @brief  发送命令到LCD（阻塞模式）
 */
void LCD_SendCmd(uint8_t cmd)
{
    LCD_DC_Clr();
    LCD_CS_Clr();
    HAL_SPI_Transmit(&LCD_SPI_HANDLE, &cmd, 1, HAL_MAX_DELAY);
    LCD_CS_Set();
}

/**
 * @brief  发送数据到LCD（阻塞模式）
 */
void LCD_SendData(uint8_t data)
{
    LCD_DC_Set();
    LCD_CS_Clr();
    HAL_SPI_Transmit(&LCD_SPI_HANDLE, &data, 1, HAL_MAX_DELAY);
    LCD_CS_Set();
}

/**
 * @brief  向LCD写入寄存器和数据
 */
void LCD_WriteReg(uint8_t reg, uint16_t data)
{
    LCD_SendCmd(reg);
    LCD_SendData(data >> 8);
    LCD_SendData(data & 0xFF);
}

/**
 * @brief  准备向LCD GRAM写入数据
 */
void LCD_WriteRAM_Prepare(void)
{
    LCD_SendCmd(lcddev.wramcmd);
}

/**
 * @brief  向LCD写入16位数据（阻塞模式）
 */
void LCD_WriteData_16Bit(uint16_t data)
{
    uint8_t tx_buffer[2];
    tx_buffer[0] = data >> 8;
    tx_buffer[1] = data & 0xFF;
    
    LCD_DC_Set();
    LCD_CS_Clr();
    HAL_SPI_Transmit(&LCD_SPI_HANDLE, tx_buffer, 2, HAL_MAX_DELAY);
    LCD_CS_Set();
}

#if LCD_USE_DMA
// ==================== DMA传输模式函数 ====================

/**
 * @brief  发送命令到LCD（DMA模式）
 */
void LCD_SendCmd_DMA(uint8_t cmd)
{
    LCD_DC_Clr();
    LCD_CS_Clr();
    HAL_SPI_Transmit_DMA(&LCD_SPI_HANDLE, &cmd, 1);
    while (HAL_SPI_GetState(&LCD_SPI_HANDLE) != HAL_SPI_STATE_READY);
    LCD_CS_Set();
}

/**
 * @brief  发送数据到LCD（DMA模式）
 */
void LCD_SendData_DMA(uint8_t data)
{
    LCD_DC_Set();
    LCD_CS_Clr();
    HAL_SPI_Transmit_DMA(&LCD_SPI_HANDLE, &data, 1);
    while (HAL_SPI_GetState(&LCD_SPI_HANDLE) != HAL_SPI_STATE_READY);
    LCD_CS_Set();
}
// 在头文件中添加
volatile uint8_t SPI_Tx_Complete = 0;
/**
 * @brief  向LCD批量写入一行16位像素数据（DMA模式）
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &LCD_SPI_HANDLE) {
        SPI_Tx_Complete = 1;
    }
}

void LCD_WriteLine_DMA(uint16_t* line_buffer, uint32_t width)
{
    SPI_Tx_Complete = 0;
    LCD_DC_Set();
    LCD_CS_Clr();
    
    HAL_SPI_Transmit_DMA(&LCD_SPI_HANDLE, (uint8_t*)line_buffer, width * 2);
    
    // 等待传输完成标志
    while (!SPI_Tx_Complete) {
        // 超时处理可以加在这里
    }
    
    LCD_CS_Set();
}

/**
 * @brief  快速清屏（DMA模式）
 */
void LCD_Clear_DMA(uint16_t color)
{
	uint16_t color_swapped = (color >> 8) | (color << 8);
    // 分配一行缓冲区
    uint16_t *line_buffer = malloc(lcddev.width * sizeof(uint16_t));
    
    if(line_buffer == NULL) {
        // 内存分配失败，回退到阻塞模式
        LCD_Clear(color_swapped);
        return;
    }
    
    // 填充颜色缓冲区
    for(uint16_t i = 0; i < lcddev.width; i++) {
        line_buffer[i] = color_swapped;
    }
    
    // 设置全屏窗口
    LCD_SetWindow(0, 0, lcddev.width, lcddev.height);
    LCD_WriteRAM_Prepare();
    
    // 逐行传输
    for(uint16_t y = 0; y < lcddev.height; y++) {
        LCD_WriteLine_DMA(line_buffer, lcddev.width);
    }
    
    free(line_buffer);
}
#endif // LCD_USE_DMA

// ==================== 通用函数 ====================

void LCD_Reset(void)
{
    LCD_RST_Clr();
    HAL_Delay(50);
    LCD_RST_Set();
    HAL_Delay(50);
}

void LCD_BackLight_On(void)
{
    LCD_LED_ON();
}

void LCD_BackLight_Off(void)
{
    LCD_LED_OFF();
}

/**
 * @brief  设置LCD显示窗口
 */
void LCD_SetWindow(uint16_t startX, uint16_t startY, uint16_t width, uint16_t height)
{
    uint16_t endX = startX + width - 1;
    uint16_t endY = startY + height - 1;
    
    // 设置列地址范围
    LCD_SendCmd(0x2A);
    LCD_SendData(startX >> 8);
    LCD_SendData(startX & 0xFF);
    LCD_SendData(endX >> 8);
    LCD_SendData(endX & 0xFF);

    // 设置行地址范围
    LCD_SendCmd(0x2B);
    LCD_SendData(startY >> 8);
    LCD_SendData(startY & 0xFF);
    LCD_SendData(endY >> 8);
    LCD_SendData(endY & 0xFF);
    
    // 开始写入显存
    LCD_WriteRAM_Prepare();
}

void LCD_SetCursor(uint16_t x, uint16_t y)
{
    LCD_SetWindow(x, y, 1, 1);
}

/**
 * @brief  通用清屏函数（自动选择模式）
 */
void LCD_Clear(uint16_t color)
{
#if LCD_USE_DMA
    

    LCD_Clear_DMA(color);
#else
    // 阻塞模式清屏
    LCD_SetWindow(0, 0, lcddev.width, lcddev.height);
    
    uint32_t total_pixels = lcddev.width * lcddev.height;
    // 例如在清屏时
    uint16_t color_swapped = (color >> 8) | (color << 8);
    for(uint32_t i = 0; i < total_pixels; i++) {
        LCD_WriteData_16Bit(color_swapped);
    }
#endif
}

// ==================== 绘图函数 ====================

/**
 * @brief  在指定位置画点
 */
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    LCD_SetWindow(x, y, 1, 1);
    LCD_WriteData_16Bit(color);
}

/**
 * @brief  画线函数
 */
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint16_t t; 
    int xerr = 0, yerr = 0, delta_x, delta_y, distance; 
    int incx, incy, uRow, uCol; 

    delta_x = x2 - x1; // 计算坐标增量 
    delta_y = y2 - y1; 
    uRow = x1; 
    uCol = y1; 
    
    if(delta_x > 0) incx = 1; // 设置单步方向 
    else if(delta_x == 0) incx = 0; // 垂直线 
    else {incx = -1; delta_x = -delta_x;} 
    
    if(delta_y > 0) incy = 1; 
    else if(delta_y == 0) incy = 0; // 水平线 
    else {incy = -1; delta_y = -delta_y;} 
    
    if(delta_x > delta_y) distance = delta_x; // 选取基本增量坐标轴 
    else distance = delta_y; 
    
    for(t = 0; t <= distance + 1; t++) // 画线输出 
    {  
        LCD_DrawPoint(uRow, uCol, color); // 画点 
        xerr += delta_x; 
        yerr += delta_y; 
        if(xerr > distance) 
        { 
            xerr -= distance; 
            uRow += incx; 
        } 
        if(yerr > distance) 
        { 
            yerr -= distance; 
            uCol += incy; 
        } 
    }  
}

/**
 * @brief  在指定位置画实心圆
 */
void LCD_DrawFilledCircle(uint16_t centerX, uint16_t centerY, uint16_t radius, uint16_t color)
{
    int16_t x = 0;
    int16_t y = radius;
    int16_t d = 1 - radius;
    int16_t deltaE = 3;
    int16_t deltaSE = -2 * radius + 5;

    // 画初始的八个点
    LCD_DrawLine(centerX - radius, centerY, centerX + radius, centerY, color);

    while (y > x)
    {
        if (d < 0) // 选择东点
        {
            d += deltaE;
            deltaE += 2;
            deltaSE += 2;
        }
        else // 选择东南点
        {
            d += deltaSE;
            deltaE += 2;
            deltaSE += 4;
            y--;
        }
        x++;

        // 画对称的八个点
        LCD_DrawLine(centerX - x, centerY + y, centerX + x, centerY + y, color);
        LCD_DrawLine(centerX - x, centerY - y, centerX + x, centerY - y, color);
        LCD_DrawLine(centerX - y, centerY + x, centerX + y, centerY + x, color);
        LCD_DrawLine(centerX - y, centerY - x, centerX + y, centerY - x, color);
    }
}

/**
 * @brief  设置LCD显示方向
 */
void LCD_SetDirection(uint8_t direction)
{
    lcddev.setxcmd = 0x2A;
    lcddev.setycmd = 0x2B;
    lcddev.wramcmd = 0x2C;
    lcddev.dir = direction;
    switch(direction)
    {
        case 0:     // 0度
            lcddev.width = LCD_WIDTH;
            lcddev.height = LCD_HEIGHT;
            LCD_SendCmd(0x36);
            LCD_SendData((1<<3)|(0<<6)|(0<<7));
            break;
            
        case 1:     // 90度
            lcddev.width = LCD_HEIGHT;
            lcddev.height = LCD_WIDTH;
            LCD_SendCmd(0x36);
            LCD_SendData((1<<3)|(0<<7)|(1<<6)|(1<<5));
            break;
            
        case 2:     // 180度
            lcddev.width = LCD_WIDTH;
            lcddev.height = LCD_HEIGHT;
            LCD_SendCmd(0x36);
            LCD_SendData((1<<3)|(1<<6)|(1<<7));
            break;
            
        case 3:     // 270度
            lcddev.width = LCD_HEIGHT;
            lcddev.height = LCD_WIDTH;
            LCD_SendCmd(0x36);
            LCD_SendData((1<<3)|(1<<7)|(1<<5));
            break;
            
        default:
            break;
    }
}

// ==================== LCD初始化函数 ====================

void LCD_Init(void)
{
    LCD_Reset();
    HAL_Delay(100);
 
    // 开始初始化LCD
    LCD_SendCmd(0xCF);
    LCD_SendData(0x00);
    LCD_SendData(0xD9);
    LCD_SendData(0X30);
    
    LCD_SendCmd(0xED);
    LCD_SendData(0x64);
    LCD_SendData(0x03);
    LCD_SendData(0X12);
    LCD_SendData(0X81);
    
    LCD_SendCmd(0xE8);
    LCD_SendData(0x85);
    LCD_SendData(0x10);
    LCD_SendData(0x7A);
    
    LCD_SendCmd(0xCB);
    LCD_SendData(0x39);
    LCD_SendData(0x2C);
    LCD_SendData(0x00);
    LCD_SendData(0x34);
    LCD_SendData(0x02);
    
    LCD_SendCmd(0xF7);
    LCD_SendData(0x20);
    
    LCD_SendCmd(0xEA);
    LCD_SendData(0x00);
    LCD_SendData(0x00);
    
    // Power Control
    LCD_SendCmd(0xC0);
    LCD_SendData(0x1B);
    
    LCD_SendCmd(0xC1);
    LCD_SendData(0x12);
    
    // VCM Control
    LCD_SendCmd(0xC5);
    LCD_SendData(0x08);
    LCD_SendData(0x26);
    
    LCD_SendCmd(0xC7);
    LCD_SendData(0XB7);
    
    // Memory Access Control
    LCD_SendCmd(0x36);
    LCD_SendData(0x08);
    
    LCD_SendCmd(0x3A);
    LCD_SendData(0x55);
    
    LCD_SendCmd(0xB1);
    LCD_SendData(0x00);
    LCD_SendData(0x1A);
    
    // Display Function Control
    LCD_SendCmd(0xB6);
    LCD_SendData(0x0A);
    LCD_SendData(0xA2);
    
    // 3Gamma Function
    LCD_SendCmd(0xF2);
    LCD_SendData(0x00);
    
    LCD_SendCmd(0x26);
    LCD_SendData(0x01);
    
    // Set Gamma
    LCD_SendCmd(0xE0);
    LCD_SendData(0x0F);
    LCD_SendData(0x1D);
    LCD_SendData(0x1A);
    LCD_SendData(0x0A);
    LCD_SendData(0x0D);
    LCD_SendData(0x07);
    LCD_SendData(0x49);
    LCD_SendData(0x66);
    LCD_SendData(0x3B);
    LCD_SendData(0x07);
    LCD_SendData(0x11);
    LCD_SendData(0x01);
    LCD_SendData(0x09);
    LCD_SendData(0x05);
    LCD_SendData(0x04);
    
    LCD_SendCmd(0XE1);
    LCD_SendData(0x00);
    LCD_SendData(0x18);
    LCD_SendData(0x1D);
    LCD_SendData(0x02);
    LCD_SendData(0x0F);
    LCD_SendData(0x04);
    LCD_SendData(0x36);
    LCD_SendData(0x13);
    LCD_SendData(0x4C);
    LCD_SendData(0x07);
    LCD_SendData(0x13);
    LCD_SendData(0x0F);
    LCD_SendData(0x2E);
    LCD_SendData(0x2F);
    LCD_SendData(0x05);
    
    // 设置显示区域
    LCD_SendCmd(0x2B);
    LCD_SendData(0x00);
    LCD_SendData(0x00);
    LCD_SendData(0x01);
    LCD_SendData(0x3F);
    
    LCD_SendCmd(0x2A);
    LCD_SendData(0x00);
    LCD_SendData(0x00);
    LCD_SendData(0x00);
    LCD_SendData(0xEF);
    
    // 退出睡眠模式
    LCD_SendCmd(0x11);
    HAL_Delay(120);
    
    // 开启显示
    LCD_SendCmd(0x29);
    
    // 设置显示方向
    LCD_SetDirection(USE_HORIZONTAL);
    
    // 开启背光并清屏
    LCD_BackLight_On();
    LCD_Clear(WHITE);  // 使用通用清屏函数
}

uint8_t GetLcdDir(void){
	return lcddev.dir;
}
