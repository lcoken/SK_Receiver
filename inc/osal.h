#ifndef __OSAL__
#define __OSAL__

#include "skhl_data_typedef.h"
#include "debug.h"
#ifndef SK_WINDOWS
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#ifdef SK_WINDOWS
typedef uint64_t (*task_entry)(void *p);
#else
typedef void* (*task_entry)(void *p);
#endif

#ifdef SK_WINDOWS
#define TASK_PRIO_HIGHEST           THREAD_PRIORITY_LOWEST
#define TASK_PRIO_ABOVE_NORMAL      THREAD_PRIORITY_ABOVE_NORMAL
#define TASK_PRIO_NORMAL            THREAD_PRIORITY_IDLE
#define TASK_PRIO_BELOW_NORMAL      THREAD_PRIORITY_BELOW_NORMAL
#else
#define TASK_PRIO_HIGHEST           99//THREAD_PRIORITY_LOWEST
#define TASK_PRIO_ABOVE_NORMAL      50//THREAD_PRIORITY_ABOVE_NORMAL
#define TASK_PRIO_NORMAL            25//THREAD_PRIORITY_IDLE
#define TASK_PRIO_BELOW_NORMAL      10//THREAD_PRIORITY_BELOW_NORMAL
#endif


typedef struct
{
    const char  *name;
    task_entry  fn;
    uint32_t    stack_size;         // 0 for default 1MB.
    uint32_t    *arg;
    uint64_t    task_id;
    uint8_t     auto_start;         // 0 -> not start auto.
    uint32_t    prio;
} task_attr;

typedef struct
{
    const char  *name;
} event_attr;

typedef struct
{
    const char  *name;
} mutex_attr;

typedef struct
{
    const char  *name;          // The name of the file or device to be created or opened.
    uint64_t    access;         // The requested access to the file or device, which can be summarized as read, write, both or neither zero).
    uint64_t    share_mode;     // The requested sharing mode of the file or device, which can be read, write, both, delete, all of these, or none
    uint64_t    creation;       // An action to take on a file or device that exists or does not exist.
    uint64_t    flag;           // The file or device attributes and flags.
} file_attr;

skhl_handle event_init(event_attr *desc);
skhl_result event_post(skhl_handle event_handle);
skhl_result event_wait(skhl_handle event_handle);
skhl_result event_destory(skhl_handle event_handle);

skhl_handle mutex_init(mutex_attr *desc);
skhl_result mutex_lock(skhl_handle mutex_handle);
skhl_result mutex_unlock(skhl_handle mutex_handle);
skhl_result mutex_destory(skhl_handle mutex_handle);

skhl_handle task_init(task_attr *desc);
skhl_result task_destory(skhl_handle task_handle);

skhl_handle file_init(file_attr *desc);
skhl_result file_close(skhl_handle file);
skhl_result file_read(skhl_handle file, uint8_t *buff, uint32_t size, int32_t *real_size);
skhl_result file_write(skhl_handle file, uint8_t *buff, uint32_t size, int32_t *real_size);
skhl_result get_file_size(skhl_handle file, uint32_t *file_size);
void skhl_sleep(uint32_t ms);


#endif

