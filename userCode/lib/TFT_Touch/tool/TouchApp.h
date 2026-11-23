#ifndef __TOUCH_APP_H__
#define __TOUCH_APP_H__

#include "Touch.h"
#include <stdint.h>

// 触摸事件类型
typedef enum {
    TOUCH_EVENT_NONE = 0,
    TOUCH_EVENT_SINGLE_CLICK,
    TOUCH_EVENT_DOUBLE_CLICK,
    TOUCH_EVENT_LONG_PRESS,
    TOUCH_EVENT_SWIPE_UP,
    TOUCH_EVENT_SWIPE_DOWN,
    TOUCH_EVENT_SWIPE_LEFT,
    TOUCH_EVENT_SWIPE_RIGHT
} TouchEventType_t;

// 触摸事件结构体
typedef struct {
    TouchEventType_t type;
    TouchPoint_t start_point;
    TouchPoint_t end_point;
    uint32_t timestamp;
} TouchEvent_t;

// 初始化触摸应用
void TouchApp_Init(void);
// 设置触摸参数配置
void TouchApp_SetConfig(uint16_t click_ms, uint16_t double_click_ms, uint16_t long_press_ms, uint16_t swipe_threshold);
// 轮询触摸事件
TouchEventType_t TouchApp_PollEvent(TouchEvent_t* event);

#endif


