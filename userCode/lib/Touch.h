#ifndef __TOUCH_H__
#define __TOUCH_H__

#include "main.h"
#include "stm32f4xx_hal.h"
#include "spi.h"

// ==== 用户可配置宏定义（如无特殊需求可不动） ====
#ifndef TOUCH_SPI_HANDLE
#define TOUCH_SPI_HANDLE hspi3
#endif
#ifndef TOUCH_CS_GPIO_Port
#define TOUCH_CS_GPIO_Port GPIOC
#endif
#ifndef TOUCH_CS_Pin
#define TOUCH_CS_Pin GPIO_PIN_8
#endif
#ifndef TOUCH_PWR1_GPIO_Port
#define TOUCH_PWR1_GPIO_Port GPIOC
#endif
#ifndef TOUCH_PWR1_Pin
#define TOUCH_PWR1_Pin GPIO_PIN_10
#endif
#ifndef TOUCH_PWR2_GPIO_Port
#define TOUCH_PWR2_GPIO_Port GPIOC
#endif
#ifndef TOUCH_PWR2_Pin
#define TOUCH_PWR2_Pin GPIO_PIN_11
#endif
#ifndef TOUCH_MIN_X
#define TOUCH_MIN_X 240
#endif
#ifndef TOUCH_MAX_X
#define TOUCH_MAX_X 3800
#endif
#ifndef TOUCH_MIN_Y
#define TOUCH_MIN_Y 320
#endif
#ifndef TOUCH_MAX_Y
#define TOUCH_MAX_Y 3800
#endif

// ==== 结构体定义 ====
typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t  pressed;
} TouchPoint_t;

typedef struct {
    float scale_x;
    float scale_y;
    int16_t offset_x;
    int16_t offset_y;
    uint16_t raw_min_x;
    uint16_t raw_max_x;
    uint16_t raw_min_y;
    uint16_t raw_max_y;
} TouchCalibration_t;

// ==== 全局变量声明 ====
extern TouchCalibration_t Touch_Calibration;

// ==== API声明 ====
void Touch_Init(void);
uint8_t Touch_Read(TouchPoint_t* point);
uint8_t Touch_ReadRaw(uint16_t* x, uint16_t* y, uint16_t* z);
uint8_t Touch_ReadRawFiltered(uint16_t* x, uint16_t* y, uint16_t* z);
void Touch_ApplyCalibration(uint16_t raw_x, uint16_t raw_y, uint16_t* cal_x, uint16_t* cal_y);
uint8_t Touch_IsPressed(uint16_t raw_x, uint16_t raw_y);
void Touch_SetCalibration(float scale_x, float scale_y, int16_t offset_x, int16_t offset_y);
void Touch_SetRawRange(uint16_t min_x, uint16_t max_x, uint16_t min_y, uint16_t max_y);

#endif


