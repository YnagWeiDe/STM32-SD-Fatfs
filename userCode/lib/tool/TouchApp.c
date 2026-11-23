#include "TouchApp.h"
#include "Touch.h"
#include <string.h>
#include <stdlib.h>
#include "math.h"
#include "main.h"
#include "stdio.h"

// 默认参数配置
#define TOUCHAPP_CLICK_MS         200
#define TOUCHAPP_DOUBLE_CLICK_MS  400
#define TOUCHAPP_LONG_PRESS_MS    800
#define TOUCHAPP_SWIPE_THRESHOLD  40

// 内部状态变量
static uint16_t s_click_ms = TOUCHAPP_CLICK_MS;
static uint16_t s_double_click_ms = TOUCHAPP_DOUBLE_CLICK_MS;
static uint16_t s_long_press_ms = TOUCHAPP_LONG_PRESS_MS;
static uint16_t s_swipe_threshold = TOUCHAPP_SWIPE_THRESHOLD;

static uint32_t last_down_time = 0;
static uint32_t last_up_time = 0;
static uint8_t click_count = 0;

// 用户需实现此函数，返回当前毫秒数
extern uint32_t TouchApp_GetTick(void);

/**
  * @brief  设置触摸参数配置
  * @param  click_ms: 单击时间阈值(ms)
  * @param  double_click_ms: 双击间隔时间(ms)
  * @param  long_press_ms: 长按时间阈值(ms)
  * @param  swipe_threshold: 滑动触发阈值
  */
void TouchApp_SetConfig(uint16_t click_ms, uint16_t double_click_ms, 
                       uint16_t long_press_ms, uint16_t swipe_threshold)
{
    s_click_ms = click_ms;
    s_double_click_ms = double_click_ms;
    s_long_press_ms = long_press_ms;
    s_swipe_threshold = swipe_threshold;
}

/**
  * @brief  初始化触摸应用
  */
void TouchApp_Init(void)
{
    last_down_time = 0;
    last_up_time = 0;
    click_count = 0;
}

/**
  * @brief  轮询处理触摸事件
  * @param  event: 触摸事件结构体指针
  * @retval 触摸事件类型
  */
TouchEventType_t TouchApp_PollEvent(TouchEvent_t* event)
{
    static uint8_t last_pressed = 0;
    static uint32_t press_start_time = 0;
    static TouchPoint_t press_start_point = {0};
    static TouchPoint_t press_end_point = {0};
    static uint8_t long_press_reported = 0;
    
    TouchPoint_t point;
    uint32_t now = TouchApp_GetTick();
    event->type = TOUCH_EVENT_NONE;

    if(Touch_Read(&point)) {
        // 触摸按下状态
        if(!last_pressed) {
            // 新的按下动作
            press_start_time = now;
            press_start_point = point;
            long_press_reported = 0;
        }
        
        // 长按检测
        if(!long_press_reported && (now - press_start_time > s_long_press_ms)) {
            event->type = TOUCH_EVENT_LONG_PRESS;
            event->start_point = press_start_point;
            event->end_point = point;
            event->timestamp = now;
            long_press_reported = 1;
            last_pressed = 1;
            return event->type;
        }
        
        last_pressed = 1;
        press_end_point = point; // 更新结束点坐标
        
    } else {
        // 触摸释放状态
        if(last_pressed) {
            uint32_t press_time = now - press_start_time;
            int16_t dx = (int16_t)press_end_point.x - (int16_t)press_start_point.x;
            int16_t dy = (int16_t)press_end_point.y - (int16_t)press_start_point.y;
            
            // 调试信息
            printf("dx:%d, dy:%d\r\n", dx, dy);
            printf("End point - x:%d, y:%d\r\n", press_end_point.x, press_end_point.y);
            
            // 滑动事件检测
            if(abs(dx) > s_swipe_threshold || abs(dy) > s_swipe_threshold) {
                if(abs(dx) > abs(dy)) {
                    // 水平滑动
                    event->type = (dx > 0) ? TOUCH_EVENT_SWIPE_LEFT : TOUCH_EVENT_SWIPE_RIGHT;
                } else {
                    // 垂直滑动
                    event->type = (dy > 0) ? TOUCH_EVENT_SWIPE_UP : TOUCH_EVENT_SWIPE_DOWN;
                }
                event->start_point = press_start_point;
                event->end_point = press_end_point;
                event->timestamp = now;
                last_pressed = 0;
                click_count = 0;
                return event->type;
            }
            
            // 单击/双击事件检测
            if(press_time < s_click_ms) {
                if(click_count == 0 || (now - last_up_time) > s_double_click_ms) {
                    click_count = 1;
                } else {
                    click_count++;
                }
                
                // 双击检测
                if(click_count == 2) {
                    event->type = TOUCH_EVENT_DOUBLE_CLICK;
                    event->start_point = press_start_point;
                    event->end_point = press_end_point;
                    event->timestamp = now;
                    click_count = 0;
                    last_up_time = now;
                    last_pressed = 0;
                    return event->type;
                }
                last_up_time = now;
            }
            // 单击确认
            else if(press_time < s_long_press_ms && !long_press_reported) {
                event->type = TOUCH_EVENT_SINGLE_CLICK;
                event->start_point = press_start_point;
                event->end_point = press_end_point;
                event->timestamp = now;
                click_count = 0;
                last_up_time = now;
                last_pressed = 0;
                return event->type;
            }
            
            last_pressed = 0;
            
        } else if(click_count == 1 && (now - last_up_time) > s_double_click_ms) {
            // 单击超时确认（未等到第二次点击）
            event->type = TOUCH_EVENT_SINGLE_CLICK;
            event->start_point = press_start_point;
            event->end_point = press_end_point;
            event->timestamp = now;
            click_count = 0;
            return event->type;
        }
    }
    
    return TOUCH_EVENT_NONE;
}

