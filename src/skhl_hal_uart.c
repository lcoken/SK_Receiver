#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "osal.h"

#include "skhl_hal_uart.h"
#include "skhl_data_typedef.h"

static struct termios Termios  = {0};

skhl_handle skhl_hal_uart_init(char *port)
{
    file_attr       file        = {0};
    skhl_handle     hal_uart    = NULL;
    struct termios  newtermios  = {0};
    uart_attr_t     attr        = {0};

    file.name       = port;
    file.flag       = O_RDWR | O_NOCTTY | O_NONBLOCK;
    hal_uart        = file_init(&file);
    if (NULL == hal_uart)
    {
        log_err("file init %s error!\n", port);
        return NULL;
    }
    log_debug("Success open %s\n", port);

    tcgetattr((int)*hal_uart, &Termios);

    attr.baud         = SPABAUD_115200;
    attr.databit      = SPADATABITS_8;
    attr.stopbit      = SPASTOPBITS_1;
    attr.parity       = SPAPARITY_NONE;
    attr.protocol     = SPAPROTOCOL_NONE;

    memset(&newtermios, 0, sizeof(struct termios));
    newtermios.c_cflag = attr.baud | attr.stopbit | attr.parity | attr.databit | CLOCAL | CREAD;
    if (attr.protocol == SPAPROTOCOL_RTS_CTS)
    {
        newtermios.c_cflag |= CRTSCTS;
    }
    else
    {
        newtermios.c_cflag |= attr.protocol;
    }
    newtermios.c_cc[VMIN] = 1;
    tcflush((int)*hal_uart, TCIOFLUSH);
    tcsetattr((int)*hal_uart, TCSANOW, &newtermios);
    tcflush((int)*hal_uart, TCIOFLUSH);
    fcntl((int)*hal_uart, F_SETFL, O_NONBLOCK);

    return (skhl_handle)hal_uart;
}


uint32_t skhl_hal_uart_read_data(skhl_handle fd, uint8_t *buff, uint32_t size)
{
    int32_t real_size = 0;
    skhl_result ret = 0;

    ret = file_read(fd, buff, size, &real_size);
    if (ret < 0)
    {
        log_warn("file read error!\n");
        return 0;
    }

    return real_size;
}

uint32_t skhl_hal_uart_write_data(skhl_handle fd, uint8_t *buff, uint32_t size)
{
    int32_t real_size = 0;
    skhl_result ret = 0;

    ret = file_write(fd, buff, size, &real_size);
    if (ret < 0)
    {
        log_warn("file write error!\n");
        return 0;
    }

    return real_size;
}

skhl_result skhl_hal_uart_close(skhl_handle fd)
{
    tcsetattr(*((int *)fd), TCSANOW, &Termios);
    file_close(fd);
    return 0;
}


