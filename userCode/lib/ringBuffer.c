
#include "ringbuffer.h"

// Initialize the ring buffer
void ringbuffer_init(ringbuffer_t *rb)
{
    // Set read and write pointers to 0
    rb->r = 0;
    rb->w = 0;
    // Clear buffer memory
    memset(rb->buffer, 0, sizeof(uint8_t) * RINGBUFFER_SIZE);
    // Initialize item count to 0
    rb->itemCount = 0;
}


// Check if the ring buffer is full
uint8_t ringbuffer_is_full(ringbuffer_t *rb)
{
    // Return 1 if itemCount equals buffer size, else 0
    return (rb->itemCount == RINGBUFFER_SIZE);
}

// Check if the ring buffer is empty
uint8_t ringbuffer_is_empty(ringbuffer_t *rb)
{
    // Return 1 if itemCount is 0, else 0
    return (rb->itemCount == 0);
}

// Write data to the ring buffer
int8_t ringbuffer_write(ringbuffer_t *rb, uint8_t *data, uint32_t num)
{
    uint32_t i = 0;
    // If buffer is full, return -1
    if(ringbuffer_is_full(rb))
        return -1;

    // Write data to buffer
    while(i < num)
    {
        if(ringbuffer_is_full(rb))
            break;
        rb->buffer[rb->w] = data[i]; // Write data and move write pointer
        rb->w = (rb->w + 1) % RINGBUFFER_SIZE; // Circular increment
        rb->itemCount++;
        i++;
    }

    return i; // Return number of bytes written
}

// Read data from the ring buffer
int8_t ringbuffer_read(ringbuffer_t *rb, uint8_t *data, uint32_t num)
{
    uint32_t i = 0;
    // If buffer is empty, return -1
    if(ringbuffer_is_empty(rb))
        return -1;

    // Read data from buffer
    while(i < num)
    {
        if(ringbuffer_is_empty(rb))
            break;
        data[i] = rb->buffer[rb->r]; // Read data and move read pointer
        rb->r = (rb->r + 1) % RINGBUFFER_SIZE; // Circular increment
        rb->itemCount--;
        i++;
    }
    return i; // Return number of bytes read
}