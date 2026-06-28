/*
 * delay.c — Implementare delay_ms folosind millis() din driver/timer
 */

#include "delay.h"
#include "timer.h"   /* millis() */

void delay_ms(uint16_t ms)
{
    uint32_t start = millis();
    while ((uint32_t)(millis() - start) < (uint32_t)ms);
}
