/*
 * timer.c — Timer0 CTC 1ms, implementare millis()
 *
 * Timer0 in modul CTC (Clear Timer on Compare):
 *   WGM01=1 → CTC mode
 *   CS01+CS00=1 → prescaler 64
 *   OCR0A = 249
 *   TIMSK0 |= OCIE0A → interrupt la match
 *
 * ISR incrementeaza ms_count la fiecare 1ms.
 * millis() returneaza o copie atomica a ms_count.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer.h"

static volatile uint32_t ms_count = 0;

void timer_init(void)
{
    /* CTC mode: WGM02=0, WGM01=1, WGM00=0 */
    TCCR0A = (1 << WGM01);

    /* Prescaler 64: CS02=0, CS01=1, CS00=1 */
    TCCR0B = (1 << CS01) | (1 << CS00);

    /*
     * OCR0A = F_CPU / (prescaler * f_tick) - 1
     *       = 16000000 / (64 * 1000) - 1 = 249
     */
    OCR0A = 249;

    /* Activeaza interrupt la compare match A */
    TIMSK0 = (1 << OCIE0A);
}

ISR(TIMER0_COMPA_vect)
{
    ms_count++;
}

/*
 * millis() — citire atomica a contorului de milisecunde.
 * Dezactiveaza intreruperile pe durata citirii pentru a evita
 * race condition pe uint32_t (4 bytes, nu atomic pe 8-bit AVR).
 */
uint32_t millis(void)
{
    uint32_t val;
    uint8_t sreg = SREG;
    __builtin_avr_cli();    /* cli() */
    val = ms_count;
    SREG = sreg;            /* restaureaza I-flag */
    return val;
}

void delay_ms(uint16_t ms)
{
    uint32_t start = millis();
    while ((uint32_t)(millis() - start) < ms);
}
