
#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include <stdint.h>
#include <string.h>

#define RINGBUFFER_SIZE 128

typedef struct {
	uint8_t buffer[RINGBUFFER_SIZE];
	uint32_t r;         // read pointer
	uint32_t w;         // write pointer
	uint32_t itemCount; // number of items in buffer
} ringbuffer_t;



// API
void ringbuffer_init(ringbuffer_t *rb);
uint8_t ringbuffer_is_full(ringbuffer_t *rb);
uint8_t ringbuffer_is_empty(ringbuffer_t *rb);
int8_t ringbuffer_write(ringbuffer_t *rb, uint8_t *data, uint32_t num);
int8_t ringbuffer_read(ringbuffer_t *rb, uint8_t *data, uint32_t num);

#endif
