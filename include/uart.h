#ifndef UART_H
#define UART_H

/*
 * uart.h — Driver USART0 bare metal
 * TX: blocat (polling pe UDRE0)
 * RX: interrupt-driven, buffer circular 64 bytes
 */

#include <stdint.h>

void    uart_init(void);

/* TX */
void    uart_putchar(char c);
void    uart_puts(const char *s);
void    uart_put_uint8(uint8_t n);
void    uart_put_int16(int16_t n);
void    uart_put_uint16(uint16_t n);
void    uart_put_float1(int16_t val_x10); /* ex: val=453 => "45.3" */

/* RX */
uint8_t uart_available(void);
char    uart_getchar(void);
uint8_t uart_readline(char *buf, uint8_t maxlen); /* citeste pana la '\n', ret lungime */

#endif /* UART_H */
