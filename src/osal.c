#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include "osal.h"

#include "skhl_data_typedef.h"

skhl_handle event_init(event_attr *desc)
{
    int32_t ret = 0;
    sem_t *event = NULL;

    event = malloc(sizeof(sem_t));
    if (NULL == event)
    {
        return NULL;
    }

    ret = sem_init(event, 0, 0);
    if (ret != 0)
    {
        free(event);
        return NULL;
    }

    return (skhl_handle)event;
}

skhl_result event_post(skhl_handle event_handle)
{
    int32_t ret = 0;

    ret = sem_post((sem_t *)event_handle);
    if (ret != 0)
    {
        return -1;
    }

    return 0;
}

skhl_result event_wait(skhl_handle event_handle)
{
    int32_t ret = 0;

    ret = sem_wait((sem_t *)event_handle);
    if (ret != 0)
    {
        return -1;
    }

    return 0;
}

skhl_result event_destory(skhl_handle event_handle)
{
    sem_destroy((sem_t *)event_handle);
    free(event_handle);
    event_handle = NULL;

    return 0;
}

skhl_handle mutex_init(mutex_attr *desc)
{
    int32_t ret = 0;
    pthread_mutex_t *mutex;

    mutex = malloc(sizeof(pthread_mutex_t));
    if (NULL == mutex)
    {
        return NULL;
    }

    ret = pthread_mutex_init(mutex, NULL);
    if (ret != 0)
    {
        free(mutex);
        return NULL;
    }

    return (skhl_handle)mutex;
}

skhl_result mutex_lock(skhl_handle mutex_handle)
{
    pthread_mutex_lock((pthread_mutex_t *)mutex_handle);
    return 0;
}

skhl_result mutex_unlock(skhl_handle mutex_handle)
{
    pthread_mutex_unlock((pthread_mutex_t *)mutex_handle);
    return 0;
}

skhl_result mutex_destory(skhl_handle mutex_handle)
{
    pthread_mutex_destroy((pthread_mutex_t *)mutex_handle);
    free(mutex_handle);
    mutex_handle = NULL;

    return 0;
}

skhl_handle task_init(task_attr *desc)
{
    int ret = 0;
    pthread_t *tinfo = NULL;
    pthread_attr_t attr;
    int policy;
    int inher;
    struct sched_param param;

    tinfo = malloc(sizeof(pthread_t));
    if (NULL == tinfo)
    {
        return NULL;
    }

    pthread_attr_init(&attr);
    if (desc->stack_size != 0)
    {
        pthread_attr_setstacksize(&attr, desc->stack_size);
    }
    pthread_attr_getinheritsched(&attr, &inher);
    if (inher != PTHREAD_EXPLICIT_SCHED)
    {
        log_warn("change sched!\n");
        inher = PTHREAD_EXPLICIT_SCHED;
    }
    pthread_attr_setinheritsched(&attr, inher);
    // 设置线程调度策略
    policy = SCHED_FIFO;
    pthread_attr_setschedpolicy(&attr, policy);
    // 设置调度参数
    param.sched_priority = desc->prio;
    pthread_attr_setschedparam(&attr, &param);

    ret = pthread_create(tinfo, &attr, desc->fn, desc->arg);
    // TODO: return code = 1 ???
    if (ret != 0)
    {
        log_err("task create err code = (%d) (%d)\n", ret, EPERM);
        pthread_attr_destroy(&attr);
        free(tinfo);
        return NULL;
    }

    /* Destroy the thread attributes object, since it is no
       longer needed */
    pthread_attr_destroy(&attr);

    /* Now join with each thread, and display its returned value */
    // ret = pthread_join(*tinfo, NULL);
    // to create detach state thread.
    ret = pthread_detach(*tinfo);
    if (ret != 0)
    {
        log_debug("HELLO! %s %d \n", __func__, __LINE__);
        free(tinfo);
        return NULL;
    }

    return (skhl_handle)tinfo;
}

skhl_result task_destory(skhl_handle task_handle)
{
    pthread_cancel((pthread_t)task_handle);
    free(task_handle);
    task_handle = NULL;

    return 0;
}

skhl_handle file_init(file_attr *desc)
{
    int *file = NULL;

    file = malloc(sizeof(int));
    if (file == NULL)
    {
        log_err("malloc file failed!\n");
        return NULL;
    }

    *file = open(desc->name, desc->flag, desc->access);
    if (*file < 0)
    {
        log_err("open file %s failed!\n", desc->name);
        free(file);
        return NULL;
    }

    return (skhl_handle)(file);
}

skhl_result file_read(skhl_handle file, uint8_t *buff, uint32_t size, int32_t *real_size)
{
    *real_size = read((*(int *)file), buff, size);
    if (*real_size < 0)
    {
        log_err("read size = %d\n", *real_size);
        return -1;
    }

    return 0;
}

skhl_result file_write(skhl_handle file, uint8_t *buff, uint32_t size, int32_t *real_size)
{
    *real_size = write((*(int *)file), buff, size);
    if (*real_size < 0)
    {
        log_err("write size = %d\n", *real_size);
        return -1;
    }

    return 0;
}

skhl_result get_file_size(skhl_handle file, uint32_t *file_size)
{
    return 0;
}

skhl_result file_close(skhl_handle file)
{
    free(file);

    return 0;
}

void skhl_sleep(uint32_t ms)
{
    usleep(1000 * ms);
}

