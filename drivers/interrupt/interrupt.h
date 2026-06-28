#ifndef INTERRUPT_H
#define INTERRUPT_H

/*
 * interrupt.h — Driver interrupt extern INT0 (buton MODE, PD2)
 * Trigger: falling edge
 * Callback: inregistrat prin interrupt_set_callback()
 */

#include <stdint.h>

typedef void (*isr_callback_t)(void);

void interrupt_int0_init(isr_callback_t cb);

#endif /* INTERRUPT_H */
