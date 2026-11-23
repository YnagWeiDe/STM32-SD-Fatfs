#include "Filter.h"


void limitValue(uint32_t *data,int size,uint32_t MinValue,uint32_t MaxValue)
{
    for(int i = 0; i < size; i++)
    {
        if(data[i] < MinValue)
            data[i] = MinValue;
        else if(data[i] > MaxValue)
            data[i] = MaxValue;
    }
}
//中值滤波
uint32_t MedianFilter(uint32_t *data,int size){
    uint32_t* copy = (uint32_t*)malloc(size*sizeof(uint32_t));
    memcpy(copy,data,size*sizeof(uint32_t));
    qsort(copy,size,sizeof(uint32_t));
    uint32_t median = (size %2 ==0)?(copy[size/2 -1] + copy[size/2])/2:copy[size/2];
    free(copy);
    return median;
}

//算术平均值滤波
uint32_t avg_value(uint32_t *data, int size)
{
    uint32_t sum = 0;
    for (int i = 0; i < size; i++)
    {
        sum += data[i];
    }
    return sum / size;
}

//?阶滞后滤波
/*
    一阶滞后滤波器是一种低通滤波器，能够有效地平滑信号，减少高频噪声的影响。
    它通过对输入信号进行采样，并将采样值与前一时刻的输出值进行比较，适?于需要平滑的曲线
    如慢速变化的传感器数据（如压?、温度）。
*/
uint32_t adcFilter(uint32_t CurrValue){
    static uint32_t LastValue;
    uint32_t tmp = (32 * CurrValue + 96 * LastValue) >>7 ;//除以128
    LastValue = tmp;
    return LastValue;
}