#include "scheduler.h"

uint8_t taskNum ;

typedef struct {
    void (*taskFunc)(void);
    uint32_t rateMs;
    uint32_t lastRun;
}task_t;

static task_t schedulerTasks[] = {

    // {led_proc,1,0},//定义一个任务数组，执行周期1ms，上次执行时间为0
    // {key_proc,10,0},
    // {uart_proc,100,0},
}

void scheduler_init(void)
{
    taskNum = sizeof(schedulerTasks)/sizeof(schedulerTasks[0]);
}

void scheduler_run(void)
{
    for(uint8_t i =0 ; i<taskNum ; i++){
        uint32_t nowTime = HAL_GetTick();//获取Hal的系统时间
        if(nowTime - schedulerTasks[i].lastRun >= schedulerTasks[i].rateMs){
            schedulerTasks[i].lastRun = nowTime;//更新上次执行时间
            schedulerTasks[i].taskFunc();//执行任务函数
        }
    }
}