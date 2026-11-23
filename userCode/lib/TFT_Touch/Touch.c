
#include "Touch.h"
#include <stdlib.h>

#define TOUCH_FILTER_SAMPLES 5

// ==== 全局变量定义 ====
TouchCalibration_t Touch_Calibration = {
    .scale_x = 1.0f,
    .scale_y = 1.0f,
    .offset_x = 0,
    .offset_y = 0,
    .raw_min_x = TOUCH_MIN_X,
    .raw_max_x = TOUCH_MAX_X,
    .raw_min_y = TOUCH_MIN_Y,
    .raw_max_y = TOUCH_MAX_Y
};

// ==== 静态内部函数声明 ====
static void TOUCH_CS_LOW(void);
static void TOUCH_CS_HIGH(void);
static void TOUCH_Delay(uint32_t delay);
static uint8_t TOUCH_SPI_TransmitReceive(uint8_t data);

// ==== 片选控制 ====
static void TOUCH_CS_LOW(void)  { HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_RESET); }
static void TOUCH_CS_HIGH(void) { HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_SET); }


// ==== 简短延时 ====
static void TOUCH_Delay(uint32_t delay)
{
    volatile uint32_t i;
    for(i = 0; i < delay; i++)
        __NOP();
}

// ==== SPI发送接收1字节 ====
static uint8_t TOUCH_SPI_TransmitReceive(uint8_t data)
{
    uint8_t rx = 0;
    HAL_SPI_TransmitReceive(&TOUCH_SPI_HANDLE, &data, &rx, 1, 10);
    return rx;
}

// ==== 读取触摸芯片原始数据（XPT2046） ====
uint8_t Touch_ReadRaw(uint16_t* x, uint16_t* y, uint16_t* z)
{
    uint16_t x_raw = 0, y_raw = 0;

    TOUCH_Delay(10);

    TOUCH_CS_LOW();
    TOUCH_Delay(2);
    TOUCH_SPI_TransmitReceive(0x90); // X通道
    TOUCH_Delay(5);
    x_raw = TOUCH_SPI_TransmitReceive(0x00) << 8;
    x_raw |= TOUCH_SPI_TransmitReceive(0x00);
    x_raw >>= 3;
    TOUCH_CS_HIGH();
    TOUCH_Delay(2);

    TOUCH_CS_LOW();
    TOUCH_Delay(2);
    TOUCH_SPI_TransmitReceive(0xD0); // Y通道
    TOUCH_Delay(1);
    y_raw = TOUCH_SPI_TransmitReceive(0x00) << 8;
    y_raw |= TOUCH_SPI_TransmitReceive(0x00);
    y_raw >>= 3;
    TOUCH_CS_HIGH();

    if(x) *x = x_raw;
    if(y) *y = y_raw;
    if(z) *z = 0;
    return 1;
}

// ==== 多次采样去极值滤波 ====
uint8_t Touch_ReadRawFiltered(uint16_t* x, uint16_t* y, uint16_t* z)
{
    uint16_t x_buf[TOUCH_FILTER_SAMPLES], y_buf[TOUCH_FILTER_SAMPLES];
    uint8_t i;
    uint16_t x_sum = 0, y_sum = 0;
    uint16_t x_max, x_min, y_max, y_min;

    for(i = 0; i < TOUCH_FILTER_SAMPLES; i++)
        if(!Touch_ReadRaw(&x_buf[i], &y_buf[i], NULL))
            return 0;

    x_max = x_min = x_buf[0];
    y_max = y_min = y_buf[0];
    for(i = 1; i < TOUCH_FILTER_SAMPLES; i++) {
        if(x_buf[i] > x_max) x_max = x_buf[i];
        if(x_buf[i] < x_min) x_min = x_buf[i];
        if(y_buf[i] > y_max) y_max = y_buf[i];
        if(y_buf[i] < y_min) y_min = y_buf[i];
    }
    for(i = 0; i < TOUCH_FILTER_SAMPLES; i++) {
        if(x_buf[i] != x_max && x_buf[i] != x_min) x_sum += x_buf[i];
        if(y_buf[i] != y_max && y_buf[i] != y_min) y_sum += y_buf[i];
    }
    if(x) *x = x_sum / (TOUCH_FILTER_SAMPLES - 2);
    if(y) *y = y_sum / (TOUCH_FILTER_SAMPLES - 2);
    if(z) *z = 0;
    return 1;
}

// ==== 校准与映射 ====
void Touch_ApplyCalibration(uint16_t raw_x, uint16_t raw_y, uint16_t* cal_x, uint16_t* cal_y)
{
    float x = (float)raw_x;
    float y = (float)raw_y;
    x = x * Touch_Calibration.scale_x + Touch_Calibration.offset_x;
    y = y * Touch_Calibration.scale_y + Touch_Calibration.offset_y;
    if(x < Touch_Calibration.raw_min_x) x = Touch_Calibration.raw_min_x;
    if(x > Touch_Calibration.raw_max_x) x = Touch_Calibration.raw_max_x;
    if(y < Touch_Calibration.raw_min_y) y = Touch_Calibration.raw_min_y;
    if(y > Touch_Calibration.raw_max_y) y = Touch_Calibration.raw_max_y;
    *cal_x = (uint16_t)((x - Touch_Calibration.raw_min_x) * 319.0f / (Touch_Calibration.raw_max_x - Touch_Calibration.raw_min_x) + 0.5f);
    *cal_y = (uint16_t)((y - Touch_Calibration.raw_min_y) * 239.0f / (Touch_Calibration.raw_max_y - Touch_Calibration.raw_min_y) + 0.5f);
    if(*cal_x > 319) *cal_x = 319;
    if(*cal_y > 239) *cal_y = 239;
}

// ==== 触摸有效性判断 ====
uint8_t Touch_IsPressed(uint16_t raw_x, uint16_t raw_y)
{
    if(raw_x < Touch_Calibration.raw_min_x || raw_x > Touch_Calibration.raw_max_x)
        return 0;
    if(raw_y < Touch_Calibration.raw_min_y || raw_y > Touch_Calibration.raw_max_y)
        return 0;
    return 1;
}

// ==== 读取当前触摸点 ====
uint8_t Touch_Read(TouchPoint_t* point)
{
    uint16_t raw_x, raw_y;
    uint16_t cal_x, cal_y;
    if(!Touch_ReadRawFiltered(&raw_x, &raw_y, NULL))
        return 0;
    Touch_ApplyCalibration(raw_x, raw_y, &cal_x, &cal_y);
    point->x = cal_x;
    point->y = cal_y;
    point->pressed = Touch_IsPressed(raw_x, raw_y);
    return point->pressed;
}

// ==== 校准参数设置 ====
void Touch_SetCalibration(float scale_x, float scale_y, int16_t offset_x, int16_t offset_y)
{
    Touch_Calibration.scale_x = scale_x;
    Touch_Calibration.scale_y = scale_y;
    Touch_Calibration.offset_x = offset_x;
    Touch_Calibration.offset_y = offset_y;
}

void Touch_SetRawRange(uint16_t min_x, uint16_t max_x, uint16_t min_y, uint16_t max_y)
{
    Touch_Calibration.raw_min_x = min_x;
    Touch_Calibration.raw_max_x = max_x;
    Touch_Calibration.raw_min_y = min_y;
    Touch_Calibration.raw_max_y = max_y;
}

// ==== 初始化 ====
void Touch_Init(void)
{
    TOUCH_CS_HIGH();
    HAL_Delay(10);
    Touch_SetCalibration(1.0f, 1.0f, 0, 0);
    Touch_SetRawRange(TOUCH_MIN_X, TOUCH_MAX_X, TOUCH_MIN_Y, TOUCH_MAX_Y);
}

