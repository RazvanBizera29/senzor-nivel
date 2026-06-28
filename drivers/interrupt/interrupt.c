/*
 * interrupt.c — Driver INT0 cu callback inregistrabil
 * Registre: EICRA (trigger), EIMSK (enable)
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "interrupt.h"

static volatile isr_callback_t int0_cb = 0;

void interrupt_int0_init(isr_callback_t cb)
{
    int0_cb = cb;
    EICRA  = (1<<ISC01);    /* falling edge pe INT0 */
    EIMSK  = (1<<INT0);     /* enable INT0 */
}

ISR(INT0_vect)
{
    if (int0_cb) int0_cb();
}
