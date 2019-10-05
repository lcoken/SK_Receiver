#include <stdio.h>
#include <string.h>

#include "skhl_data_typedef.h"
#include "debug.h"

void skhl_print_str(char *str, uint8_t *buff, uint32_t len)
{
    log_info("%s size = %d\n", str, len);

    for (uint32_t i = 0; i < len; i++)
    {
        log_info("0x%02x ", buff[i]);
        if (i % 16 == 15)
        {
            log_info("\n");
        }
    }
    log_info("\n");
    return;
}

