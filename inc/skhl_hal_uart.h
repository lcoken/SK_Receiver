#ifndef __SKHL_HAL_UART_H__
#define __SKHL_HAL_UART_H__

#include <termios.h>

typedef enum SerialBaud {
    SPABAUD_50 = B50, 
    SPABAUD_110 = B110, 
    SPABAUD_300 = B300, 
    SPABAUD_600 = B600,
    SPABAUD_1200 = B1200, 
    SPABAUD_2400 = B2400, 
    SPABAUD_4800 = B4800,
    SPABAUD_9600 = B9600, 
    SPABAUD_19200 = B19200,
    SPABAUD_38400 = B38400, 
    SPABAUD_57600 = B57600, 
    SPABAUD_115200 = B115200 
} serial_baud_e;

typedef enum SerialDatabits {
    SPADATABITS_5 = CS5, 
    SPADATABITS_6 = CS6, 
    SPADATABITS_7 = CS7, 
    SPADATABITS_8 = CS8 
} serial_databit_e;

typedef enum SerialStopbits {
    SPASTOPBITS_1 = 0, 
    SPASTOPBITS_2 = CSTOPB 
} serial_stopbit_e;

typedef enum SerialParity {
    SPAPARITY_NONE = 0, 
    SPAPARITY_ODD = PARODD | PARENB, 
    SPAPARITY_EVEN = PARENB 
} serial_parity_e;

typedef enum SerialProtocol {
    SPAPROTOCOL_NONE = 0, 
    SPAPROTOCOL_RTS_CTS = 9999,
    SPAPROTOCOL_XON_XOFF = IXOFF | IXON,
} serial_protol_e;

typedef struct
{
    serial_baud_e       baud;
    serial_databit_e    databit;
    serial_stopbit_e    stopbit;
    serial_parity_e     parity;
    serial_protol_e     protocol;
} uart_attr_t;


skhl_handle skhl_hal_uart_init(char *port);
uint32_t skhl_hal_uart_read_data(skhl_handle fd, uint8_t *buff, uint32_t size);
uint32_t skhl_hal_uart_write_data(skhl_handle fd, uint8_t *buff, uint32_t size);
skhl_result skhl_hal_uart_close(skhl_handle fd);

#endif


