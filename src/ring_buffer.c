#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "osal.h"

#include "ring_buffer.h"

skhl_result ring_buffer_init(ring_buffer_t *desc, uint8_t *buff, uint32_t size)
{
    mutex_attr mutex = {0};

    if ((size == 0) || (NULL == buff) || (NULL == desc))
    {
        return -1;
    }

    desc->base_addr = buff;
    desc->w_addr    = 0;
    desc->r_addr    = 0;
    desc->data_size = 0;
    desc->buffer_size = size;

    mutex.name = "ring_buffer";
    desc->mutex = mutex_init(&mutex);
    if (desc->mutex == NULL)
    {
        log_err("mutex create failed!\n");
        return -1;
    }

    return 0;
}

skhl_result ring_buffer_push(ring_buffer_t *desc, uint8_t *buff, uint32_t size)
{
    uint32_t left = 0;

    if ((size == 0) || (NULL == buff) || (NULL == desc))
    {
        return -1;
    }

    if (size > (desc->buffer_size - desc->data_size))
    {
        log_err("not enough buffer!\n");
        return -1;
    }

    mutex_lock(desc->mutex);

    // if no need to return to head.
    if ((desc->w_addr + size) <= desc->buffer_size)
    {
        memcpy((desc->base_addr + desc->w_addr), buff, size);
        desc->w_addr += size;
    }
    else
    {
        left = (desc->buffer_size - desc->w_addr);
        memcpy((desc->base_addr + desc->w_addr), buff, left);
        memcpy(desc->base_addr, (buff + left), (size - left));
        desc->w_addr = (size - left);
    }
    desc->data_size += size;

    mutex_unlock(desc->mutex);

    return 0;
}


skhl_result ring_buffer_pop(ring_buffer_t *desc, uint8_t *buff, uint32_t size)
{
    uint32_t left = 0;

    if ((size == 0) || (NULL == buff) || (NULL == desc))
    {
        return -1;
    }

    mutex_lock(desc->mutex);

    // if no need to return to head.
    if ((desc->r_addr + size) <= desc->buffer_size)
    {
        memcpy(buff, (desc->base_addr + desc->r_addr), size);
        desc->r_addr += size;
    }
    else
    {
        left = (desc->buffer_size - desc->r_addr);
        memcpy(buff, (desc->base_addr + desc->r_addr), left);
        memcpy((buff + left), desc->base_addr, (size - left));
        desc->r_addr = (size - left);
    }
    desc->data_size -= size;

    mutex_unlock(desc->mutex);

    return 0;
}

uint32_t ring_buffer_data_size(ring_buffer_t *desc)
{
    uint32_t data_size = 0;

    mutex_lock(desc->mutex);
    data_size = desc->data_size;
    mutex_unlock(desc->mutex);

    return data_size;
}

skhl_result ring_buffer_destory(ring_buffer_t *desc)
{
    if (NULL == desc)
    {
        return -1;
    }

    desc->base_addr = 0;
    desc->w_addr    = 0;
    desc->r_addr    = 0;
    desc->data_size = 0;
    desc->buffer_size = 0;
    mutex_destory(desc->mutex);

    return 0;
}

