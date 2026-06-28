#ifndef UART_H
#define UART_H

/*
 * uart.h — Driver USART0 bare metal, ATmega328P
 * TX: polling pe UDRE0
 * RX: interrupt-driven, buffer circular (NANO_UART_RXBUF_SZ bytes)
 */

#include <stdint.h>

void    uart_init(void);

/* TX */
void    uart_putchar(char c);
void    uart_puts(const char *s);
void    uart_put_uint16(uint16_t n);
void    uart_put_int16(int16_t n);
void    uart_put_float1(int16_t val_x10);   /* 453 → "45.3" */

/* RX */
uint8_t uart_available(void);
char    uart_getchar(void);

#endif /* UART_H */
