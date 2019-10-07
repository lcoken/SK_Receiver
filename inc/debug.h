#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "skhl_data_typedef.h"

#define __DEBUG

#ifdef __DEBUG
    #define DEBUG(format, ...) printf(format, ##__VA_ARGS__)
#else
    #define DEBUG(format, ...)
#endif

//定义日志级别
enum LOG_LEVEL {
    LOG_LEVEL_OFF=0,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_ALL,
};

static uint32_t log_level = LOG_LEVEL_ERR;

#define NONE            "\033[m"
#define RED             "\033[0;32;31m"
#define GREEN           "\033[0;32;32m"
#define BLUE            "\033[0;32;34m"
#define CYAN            "\033[0;36m"
#define YELLOW          "\033[1;33m"

#define log_fatal(format, ...) \
    do { \
         if (log_level >= LOG_LEVEL_FATAL)\
           DEBUG(format, ##__VA_ARGS__);\
    } while (0)

#define log_err(format, ...) \
    do { \
         if (log_level >= LOG_LEVEL_ERR)\
           DEBUG(format, ##__VA_ARGS__);\
    } while (0)

#define log_warn(format, ...) \
    do { \
         if (log_level >= LOG_LEVEL_WARN)\
           DEBUG(format, ##__VA_ARGS__);\
    } while (0)

#define log_info(format, ...) \
    do { \
         if (log_level >= LOG_LEVEL_INFO)\
           DEBUG(format, ##__VA_ARGS__);\
    } while (0)

#define log_debug(format, ...) \
    do { \
         if (log_level >= LOG_LEVEL_ALL)\
           DEBUG(format, ##__VA_ARGS__ );\
    } while (0)

void skhl_print_str(char *str, uint8_t *buff, uint32_t len);

#endif


