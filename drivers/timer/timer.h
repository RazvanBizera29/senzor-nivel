#ifndef TIMER_H
#define TIMER_H

/*
 * timer.h — Timer0 CTC 1ms tick, millis()
 *
 * Timer0 CTC: prescaler=64, OCR0A=249
 * Tick = 16MHz / 64 / 250 = 1000 Hz = 1ms
 */

#include <stdint.h>

void     timer_init(void);
uint32_t millis(void);

#endif /* TIMER_H */
