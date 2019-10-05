#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include "skhl_data_typedef.h"

typedef struct
{
    uint8_t     *base_addr;
    uint32_t    buffer_size;
    uint32_t    w_addr;
    uint32_t    r_addr;
    uint32_t    data_size;
    skhl_handle mutex;
} ring_buffer_t;

skhl_result ring_buffer_init(ring_buffer_t *desc, uint8_t *buff, uint32_t size);
skhl_result ring_buffer_push(ring_buffer_t *desc, uint8_t *buff, uint32_t size);
skhl_result ring_buffer_pop(ring_buffer_t *desc, uint8_t *buff, uint32_t size);
uint32_t ring_buffer_data_size(ring_buffer_t *desc);
skhl_result ring_buffer_destory(ring_buffer_t *desc);


#endif

