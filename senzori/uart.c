/*
 * uart.c — Driver USART0 bare metal, ATmega328P
 *
 * Registre folosite:
 *   UBRR0H/L  — baud rate
 *   UCSR0A    — status (UDRE0 = TX ready, RXC0 = RX complete)
 *   UCSR0B    — enable TX, RX, RX interrupt (RXCIE0)
 *   UCSR0C    — format: 8N1
 *   UDR0      — data register
 */

#include <avr/interrupt.h>
#include "uart.h"
#include "config.h"

/* ===== Buffer RX circular ===== */
#define RX_MASK (UART_RX_BUFSIZE - 1)  /* UART_RX_BUFSIZE trebuie sa fie putere a lui 2 */

static volatile char     rx_buf[UART_RX_BUFSIZE];
static volatile uint8_t  rx_head = 0;
static volatile uint8_t  rx_tail = 0;

/* ===== Initializare ===== */
void uart_init(void)
{
    /* Baud rate: UBRR = F_CPU / (16 * BAUD) - 1 */
    UBRR0H = (uint8_t)(UART_UBRR >> 8);
    UBRR0L = (uint8_t)(UART_UBRR);

    /* U2X0 = 0 (normal speed), UCSR0A default */
    UCSR0A = 0x00;

    /* Enable RX, TX, RX interrupt */
    UCSR0B = (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0);

    /* 8 data bits, 1 stop bit, no parity (8N1) */
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

/* ===== ISR RX ===== */
ISR(USART_RX_vect)
{
    char c = UDR0;
    uint8_t next = (rx_head + 1) & RX_MASK;
    if (next != rx_tail) {          /* buffer-ul nu e plin */
        rx_buf[rx_head] = c;
        rx_head = next;
    }
    /* daca plin: caracter pierdut (intentionat — nu blocam ISR) */
}

/* ===== TX ===== */
void uart_putchar(char c)
{
    while (!(UCSR0A & (1 << UDRE0)));   /* asteapta TX buffer liber */
    UDR0 = c;
}

void uart_puts(const char *s)
{
    while (*s) uart_putchar(*s++);
}

void uart_put_uint8(uint8_t n)
{
    if (n >= 100) uart_putchar('0' + n / 100);
    if (n >=  10) uart_putchar('0' + (n / 10) % 10);
    uart_putchar('0' + n % 10);
}

void uart_put_uint16(uint16_t n)
{
    char buf[6];
    uint8_t i = 0;
    if (n == 0) { uart_putchar('0'); return; }
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (i--) uart_putchar(buf[i]);
}

void uart_put_int16(int16_t n)
{
    if (n < 0) { uart_putchar('-'); n = -n; }
    uart_put_uint16((uint16_t)n);
}

/* val_x10: nivel stocat ca int16 x10 (ex: 45.3% -> val=453) */
void uart_put_float1(int16_t val_x10)
{
    if (val_x10 < 0) { uart_putchar('-'); val_x10 = -val_x10; }
    uart_put_uint16(val_x10 / 10);
    uart_putchar('.');
    uart_putchar('0' + (val_x10 % 10));
}

/* ===== RX ===== */
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

/*
 * uart_readline: citeste pana la '\n' sau maxlen-1 caractere.
 * Returneaza numarul de caractere scrise (fara '\0').
 * Non-blocking: daca nu exista '\n' in buffer, returneaza 0.
 */
uint8_t uart_readline(char *buf, uint8_t maxlen)
{
    uint8_t count = 0;
    while (uart_available() && count < maxlen - 1) {
        char c = uart_getchar();
        if (c == '\r') continue;
        if (c == '\n') {
            buf[count] = '\0';
            return count;
        }
        buf[count++] = c;
    }
    buf[count] = '\0';
    return 0;   /* '\n' inca nu a venit */
}
