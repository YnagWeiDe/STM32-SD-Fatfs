#ifndef __BSP_H__
#define __BSP_H__

#include "main.h"
#include "ringbuffer.h"

typedef struct {
    uint32_t w;
    uint32_t r;
    uint8_t buffer[RINGBUFFER_SIZE];
    uint8_t itemCount;
    } ringbuffer_t;

    extern ringbuffer_t usart_rb;

#endif
