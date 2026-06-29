#ifndef BSP_NANO_H
#define BSP_NANO_H

/*
 * nano.h — Board Support Package: Arduino Nano (ATmega328P @ 16MHz)
 *
 * Definitii specifice placii: clock, pini fizici (PORT/DDR/PIN + bit),
 * parametri periferice. Nu contine logica aplicatiei.
 */

#include <avr/io.h>

/* ================================================================
 * CLOCK
 * ================================================================ */
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

/* ================================================================
 * UART — USART0
 * ================================================================ */
#define NANO_UART_BAUD      9600UL
#define NANO_UART_UBRR      ((F_CPU / (16UL * NANO_UART_BAUD)) - 1)  /* 103 */
#define NANO_UART_RXBUF_SZ  64   /* trebuie sa fie putere a lui 2 */

/* ================================================================
 * I2C — TWI (SDA=A4/PC4, SCL=A5/PC5)
 * ================================================================ */
#define NANO_I2C_SCL_KHZ    400UL
#define NANO_I2C_TWBR       ((F_CPU / (NANO_I2C_SCL_KHZ * 1000UL) - 16UL) / 2UL)  /* 12 */

/* ================================================================
 * ADC
 * ================================================================ */
/* Prescaler 128: 16MHz/128 = 125kHz (recomandat 50-200kHz) */
#define NANO_ADC_PRESCALER  ((1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0))
/* Referinta: AVcc */
#define NANO_ADC_REFS       (1<<REFS0)

/* ================================================================
 * PINI FIZICI — Water Level System
 *
 * Conventie: NANO_PIN_<NUME>_{DDR,PORT,PIN,BIT}
 * ================================================================ */

/* Senzor nivel HW-038 — A0 (ADC0 / PC0) */
#define NANO_PIN_SENSOR_CH  0   /* canal ADC */

/* Releu pompa umplere — D7 (PD7) */
#define NANO_PIN_RELAY_DDR  DDRD
#define NANO_PIN_RELAY_PORT PORTD
#define NANO_PIN_RELAY_BIT  PD7

/* Releu pompa golire — D3 (PD3) */
#define NANO_PIN_DRAIN_DDR  DDRD
#define NANO_PIN_DRAIN_PORT PORTD
#define NANO_PIN_DRAIN_BIT  PD3

/* Buzzer activ — D6 (PD6) */
#define NANO_PIN_BUZZ_DDR   DDRD
#define NANO_PIN_BUZZ_PORT  PORTD
#define NANO_PIN_BUZZ_BIT   PD6

/* Buton MODE — D2 (PD2 = INT0, falling edge interrupt) */
#define NANO_PIN_BTN_MODE_DDR  DDRD
#define NANO_PIN_BTN_MODE_PORT PORTD
#define NANO_PIN_BTN_MODE_PIN  PIND
#define NANO_PIN_BTN_MODE_BIT  PD2

/* Buton UP — D4 (PD4) */
#define NANO_PIN_BTN_UP_DDR    DDRD
#define NANO_PIN_BTN_UP_PORT   PORTD
#define NANO_PIN_BTN_UP_PIN    PIND
#define NANO_PIN_BTN_UP_BIT    PD4

/* Buton DOWN — D5 (PD5) */
#define NANO_PIN_BTN_DN_DDR    DDRD
#define NANO_PIN_BTN_DN_PORT   PORTD
#define NANO_PIN_BTN_DN_PIN    PIND
#define NANO_PIN_BTN_DN_BIT    PD5

/* LED Rosu — D8 (PB0) nivel < 20% */
#define NANO_PIN_LED_R_DDR  DDRB
#define NANO_PIN_LED_R_PORT PORTB
#define NANO_PIN_LED_R_BIT  PB0

/* LED Galben — D9 (PB1) nivel 20-80% */
#define NANO_PIN_LED_Y_DDR  DDRB
#define NANO_PIN_LED_Y_PORT PORTB
#define NANO_PIN_LED_Y_BIT  PB1

/* LED Verde — D10 (PB2) nivel > 80% */
#define NANO_PIN_LED_G_DDR  DDRB
#define NANO_PIN_LED_G_PORT PORTB
#define NANO_PIN_LED_G_BIT  PB2

/* ================================================================
 * OLED SSD1306 (I2C)
 * ================================================================ */
#define NANO_OLED_I2C_ADDR  0x3C

#endif /* BSP_NANO_H */
