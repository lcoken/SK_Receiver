#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "osal.h"

#include "skhl_data_typedef.h"
#include "skhl_comm_core.h"

struct termios Termios  = {0};

static skhl_handle skhl_init_uart(comm_attr_t *desc)
{
    file_attr file          = {0};
    skhl_handle comm_device = NULL;
    comm_attr_t *desc_attr  = desc;
    struct termios newtermios = {0};

    if (desc_attr ==NULL)
    {
        log_err("error param!\n");
        return NULL;
    }

    file.name       = desc->name;
    file.flag       = O_RDWR | O_NOCTTY | O_NONBLOCK;
    comm_device     = file_init(&file);
    if (NULL == comm_device)
    {
        log_err("file init %s error!\n", desc->name);
        return NULL;
    }
    log_debug("Success open %s\n", desc->name);

    tcgetattr((int)*comm_device, &Termios);

    desc->attr.baud         = SPABAUD_115200;
    desc->attr.databit      = SPADATABITS_8;
    desc->attr.stopbit      = SPASTOPBITS_1;
    desc->attr.parity       = SPAPARITY_NONE;
    desc->attr.protocol     = SPAPROTOCOL_NONE;

    memset(&newtermios, 0, sizeof(struct termios));
    newtermios.c_cflag = desc->attr.baud | desc->attr.stopbit | desc->attr.parity | desc->attr.databit | CLOCAL | CREAD;
    if (desc->attr.protocol == SPAPROTOCOL_RTS_CTS)
    {
        newtermios.c_cflag |= CRTSCTS;
    }
    else
    {
        newtermios.c_cflag |= desc->attr.protocol;
    }
    newtermios.c_cc[VMIN] = 1;
    tcflush((int)*comm_device, TCIOFLUSH);
    tcsetattr((int)*comm_device, TCSANOW, &newtermios);
    tcflush((int)*comm_device, TCIOFLUSH);
    fcntl((int)*comm_device, F_SETFL, O_NONBLOCK);

    return (skhl_handle)comm_device;
}

static uint32_t skhl_read_data(skhl_handle fd, uint8_t *buff, uint32_t size)
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

static uint32_t skhl_write_data(skhl_handle fd, uint8_t *buff, uint32_t size)
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

static skhl_result skhl_close_uart(skhl_handle fd)
{
    tcsetattr(*((int *)fd), TCSANOW, &Termios);
    file_close(fd);
    return 0;
}

skhl_opt_t uart_op = {
    .name       = "uart",
    .init       = skhl_init_uart,
    .read       = skhl_read_data,
    .write      = skhl_write_data,
    .destory    = skhl_close_uart,
};

skhl_result skhl_comm_uart_init(void)
{
    return skhl_register_comm_device(&uart_op);
}

skhl_result skhl_comm_uart_destory(void)
{
    return skhl_unregister_comm_device(&uart_op);
}

