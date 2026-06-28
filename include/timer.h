#ifndef TIMER_H
#define TIMER_H

/*
 * timer.h — Timer0 CTC, 1ms tick (millis)
 *
 * Timer0 OCR0A CTC:
 *   Prescaler = 64, OCR0A = 249
 *   Tick = 16MHz / 64 / 250 = 1000 Hz = 1ms
 */

#include <stdint.h>

void     timer_init(void);
uint32_t millis(void);
void     delay_ms(uint16_t ms);

#endif /* TIMER_H */
