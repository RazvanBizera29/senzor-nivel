/*
 * buzzer.c — Driver buzzer activ non-blocking
 * GPIO controlat prin macro-uri din gpio.h / bsp.h
 */

#include "buzzer.h"
#include "gpio.h"
#include "timer.h"
#include "bsp.h"

static buzzer_mode_t cur_mode  = BUZZER_OFF;
static uint8_t       buz_state = 0;
static uint32_t      t_last    = 0;

void buzzer_init(void)
{
    GPIO_LOW(NANO_PIN_BUZZ_PORT, NANO_PIN_BUZZ_BIT);
}

void buzzer_set_mode(buzzer_mode_t mode)
{
    if (mode != cur_mode) {
        cur_mode  = mode;
        buz_state = 0;
        GPIO_LOW(NANO_PIN_BUZZ_PORT, NANO_PIN_BUZZ_BIT);
        t_last = millis();
    }
}

void buzzer_update(void)
{
    if (cur_mode == BUZZER_OFF) {
        GPIO_LOW(NANO_PIN_BUZZ_PORT, NANO_PIN_BUZZ_BIT);
        return;
    }

    uint16_t on_ms  = (cur_mode == BUZZER_FAST) ?  80 : 200;
    uint16_t off_ms = (cur_mode == BUZZER_FAST) ? 120 : 800;
    uint32_t now    = millis();
    uint32_t elapsed = (uint32_t)(now - t_last);

    if (!buz_state && elapsed >= off_ms) {
        buz_state = 1;
        GPIO_HIGH(NANO_PIN_BUZZ_PORT, NANO_PIN_BUZZ_BIT);
        t_last = now;
    } else if (buz_state && elapsed >= on_ms) {
        buz_state = 0;
        GPIO_LOW(NANO_PIN_BUZZ_PORT, NANO_PIN_BUZZ_BIT);
        t_last = now;
    }
}
