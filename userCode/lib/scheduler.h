#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <stdint.h>

// 任务函数原型（需实现）
// void led_proc(void);
// void key_proc(void);
// void uart_proc(void);

// 对外变量
extern uint8_t taskNum;

// 调度器 API
void scheduler_init(void);
void scheduler_run(void);

#endif

