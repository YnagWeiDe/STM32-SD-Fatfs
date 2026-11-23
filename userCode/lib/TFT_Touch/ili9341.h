#ifndef __ILI9341_H__
#define __ILI9341_H__

#include <stdint.h>
#include "main.h"             
#include "spi.h"              
#include "gpio.h"             
#include "stm32f4xx_hal.h"  

// 工作模式选择
#define LCD_USE_DMA          1  // 1:使用DMA模式 0:使用阻塞模式

// LCD尺寸定义
#define LCD_WIDTH           240
#define LCD_HEIGHT          320

// SPI句柄宏定义
#define LCD_SPI_HANDLE      hspi2

// 定义GPIO端口和引脚
#define LCD_RST_GPIO_Port   GPIOD
#define LCD_RST_Pin         GPIO_PIN_4
#define LCD_DC_GPIO_Port    GPIOD    
#define LCD_DC_Pin          GPIO_PIN_5
#define LCD_CS_GPIO_Port    GPIOD
#define LCD_CS_Pin          GPIO_PIN_6
#define LCD_LED_GPIO_Port   GPIOD
#define LCD_LED_Pin         GPIO_PIN_7

// 控制信号宏定义
#define LCD_RST_Clr()       HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET)
#define LCD_RST_Set()       HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET)
                                          
#define LCD_DC_Clr()        HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET)
#define LCD_DC_Set()        HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET)
                                          
#define LCD_CS_Clr()        HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET)
#define LCD_CS_Set()        HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET)
                                          
#define LCD_LED_ON()        HAL_GPIO_WritePin(LCD_LED_GPIO_Port, LCD_LED_Pin, GPIO_PIN_SET)
#define LCD_LED_OFF()       HAL_GPIO_WritePin(LCD_LED_GPIO_Port, LCD_LED_Pin, GPIO_PIN_RESET)

//LCD重要参数集
typedef struct  
{										    
    uint16_t width;         //LCD 宽度
    uint16_t height;        //LCD 高度
    uint16_t id;            //LCD ID
    uint8_t  dir;           //LCD的方向	
    uint16_t wramcmd;       //开始写gram指令
    uint16_t setxcmd;       //设置x坐标指令
    uint16_t setycmd;       //设置y坐标指令	 
		 
}_lcd_dev; 	

//LCD参数
extern _lcd_dev lcddev;

// 显示方向定义
#define USE_HORIZONTAL      0  // 0-0度 1-90度 2-180度 3-270度

// 颜色定义
#define WHITE          0xFFFF
#define BLACK          0x0000	  
#define BLUE           0x001F  
#define BRED           0XF81F
#define GRED           0XFFE0
#define GBLUE          0X07FF
#define RED            0xF800
#define MAGENTA        0xF81F
#define GREEN          0x07E0
#define CYAN           0x7FFF
#define YELLOW         0xFFE0
#define BROWN          0XBC40
#define BRRED          0XFC07
#define GRAY           0X8430

// 基础函数
void LCD_Init(void);
void LCD_Reset(void);
void LCD_SetDirection(uint8_t direction);

// 背光控制
void LCD_BackLight_On(void);
void LCD_BackLight_Off(void);

// 窗口和光标设置
void LCD_SetWindow(uint16_t startX, uint16_t startY, uint16_t width, uint16_t height);
void LCD_SetCursor(uint16_t x, uint16_t y);

// 数据传输函数 - 阻塞模式
void LCD_SendCmd(uint8_t cmd);
void LCD_SendData(uint8_t data);
void LCD_WriteReg(uint8_t reg, uint16_t data);
void LCD_WriteData_16Bit(uint16_t data);
void LCD_WriteRAM_Prepare(void);

// 数据传输函数 - DMA模式
#if LCD_USE_DMA
void LCD_SendCmd_DMA(uint8_t cmd);
void LCD_SendData_DMA(uint8_t data);
void LCD_WriteLine_DMA(uint16_t* line_buffer, uint32_t width);
void LCD_Clear_DMA(uint16_t color);
#endif

// 通用绘图函数（自动选择模式）
void LCD_Clear(uint16_t color);
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color);
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_DrawFilledCircle(uint16_t centerX, uint16_t centerY, uint16_t radius, uint16_t color);

uint8_t GetLcdDir(void);





#endif // __ILI9341_H__


