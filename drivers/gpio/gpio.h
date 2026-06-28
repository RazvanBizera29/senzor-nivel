#ifndef GPIO_H
#define GPIO_H

/*
 * gpio.h — Abstractie GPIO pentru ATmega328P
 *
 * Lucreaza direct cu registrele DDR/PORT/PIN.
 * Foloseste macro-urile din bsp/nano.h pentru definitiile pinilor.
 */

#include <avr/io.h>
#include <stdint.h>

/* Configurare directie */
#define GPIO_OUTPUT(ddr, bit)   ((ddr)  |=  (1U << (bit)))
#define GPIO_INPUT(ddr, bit)    ((ddr)  &= ~(1U << (bit)))
#define GPIO_PULLUP(port, bit)  ((port) |=  (1U << (bit)))

/* Scriere */
#define GPIO_HIGH(port, bit)    ((port) |=  (1U << (bit)))
#define GPIO_LOW(port, bit)     ((port) &= ~(1U << (bit)))
#define GPIO_TOGGLE(port, bit)  ((port) ^=  (1U << (bit)))

/* Citire */
#define GPIO_READ(pin, bit)     (((pin) >> (bit)) & 1U)

/* Initializare pini specifici proiectului */
void gpio_init(void);

#endif /* GPIO_H */
