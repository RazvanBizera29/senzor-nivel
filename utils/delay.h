#ifndef DELAY_H
#define DELAY_H

/*
 * delay.h — Delay bazat pe Timer0 millis()
 * Necesita timer_init() + sei() apelate anterior.
 */

#include <stdint.h>

void delay_ms(uint16_t ms);

#endif /* DELAY_H */
