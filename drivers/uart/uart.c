/*
 * uart.c — Driver USART0 bare metal, ATmega328P
 *
 * Registre: UBRR0H/L, UCSR0A/B/C, UDR0
 * ISR: USART_RX_vect — scrie in buffer circular
 */

#include <avr/interrupt.h>
#include "uart.h"
#include "bsp.h"

/* Buffer RX circular — dimensiunea din nano.h (trebuie putere a lui 2) */
#define RX_MASK  (NANO_UART_RXBUF_SZ - 1)

static volatile char    rx_buf[NANO_UART_RXBUF_SZ];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;

void uart_init(void)
{
    UBRR0H = (uint8_t)(NANO_UART_UBRR >> 8);
    UBRR0L = (uint8_t)(NANO_UART_UBRR);
    UCSR0A = 0x00;                                          /* normal speed */
    UCSR0B = (1<<RXCIE0) | (1<<RXEN0) | (1<<TXEN0);       /* RX int + TX/RX en */
    UCSR0C = (1<<UCSZ01) | (1<<UCSZ00);                   /* 8N1 */
}

ISR(USART_RX_vect)
{
    char c = UDR0;
    uint8_t next = (rx_head + 1) & RX_MASK;
    if (next != rx_tail) {
        rx_buf[rx_head] = c;
        rx_head = next;
    }
}

/* ---- TX ---- */
void uart_putchar(char c)
{
    while (!(UCSR0A & (1<<UDRE0)));
    UDR0 = c;
}

void uart_puts(const char *s)
{
    while (*s) uart_putchar(*s++);
}

void uart_put_uint16(uint16_t n)
{
    char buf[6];
    uint8_t i = 0;
    if (n == 0) { uart_putchar('0'); return; }
    while (n > 0) { buf[i++] = (char)('0' + n % 10); n /= 10; }
    while (i--) uart_putchar(buf[i]);
}

void uart_put_int16(int16_t n)
{
    if (n < 0) { uart_putchar('-'); n = (int16_t)(-n); }
    uart_put_uint16((uint16_t)n);
}

/* val_x10: nivel × 10, ex: 453 → "45.3" */
void uart_put_float1(int16_t val_x10)
{
    if (val_x10 < 0) { uart_putchar('-'); val_x10 = (int16_t)(-val_x10); }
    uart_put_uint16((uint16_t)(val_x10 / 10));
    uart_putchar('.');
    uart_putchar((char)('0' + val_x10 % 10));
}

/* ---- RX ---- */
uint8_t uart_available(void)
{
    return (rx_head != rx_tail);
}

char uart_getchar(void)
{
    while (!uart_available());
    char c = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1) & RX_MASK;
    return c;
}
