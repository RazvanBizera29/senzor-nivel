/*
 * timer.c — Timer0 CTC 1ms, millis() atomic
 * Registre: TCCR0A/B, OCR0A, TIMSK0
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer.h"

static volatile uint32_t ms_count = 0;

void timer_init(void)
{
    TCCR0A = (1<<WGM01);               /* CTC mode */
    TCCR0B = (1<<CS01) | (1<<CS00);   /* prescaler 64 */
    OCR0A  = 249;                      /* 16MHz/64/250 = 1kHz */
    TIMSK0 = (1<<OCIE0A);             /* interrupt on compare match A */
}

ISR(TIMER0_COMPA_vect)
{
    ms_count++;
}

uint32_t millis(void)
{
    uint32_t val;
    uint8_t sreg = SREG;
    __builtin_avr_cli();
    val = ms_count;
    SREG = sreg;
    return val;
}
