/*
 * gpio.c — Initializare GPIO specifica proiectului
 *
 * Configureaza toti pinii din nano.h la startup:
 * releu, buzzer (iesiri OFF), butoane (intrare + pull-up intern).
 */

#include "gpio.h"
#include "bsp.h"

void gpio_init(void)
{
    /* Releu pompa: iesire, initial LOW (oprit) */
    GPIO_OUTPUT(NANO_PIN_RELAY_DDR, NANO_PIN_RELAY_BIT);
    GPIO_LOW(NANO_PIN_RELAY_PORT,   NANO_PIN_RELAY_BIT);

    /* Buzzer: iesire, initial LOW (mut) */
    GPIO_OUTPUT(NANO_PIN_BUZZ_DDR, NANO_PIN_BUZZ_BIT);
    GPIO_LOW(NANO_PIN_BUZZ_PORT,   NANO_PIN_BUZZ_BIT);

    /* Buton MODE (INT0): intrare + pull-up */
    GPIO_INPUT(NANO_PIN_BTN_MODE_DDR,   NANO_PIN_BTN_MODE_BIT);
    GPIO_PULLUP(NANO_PIN_BTN_MODE_PORT, NANO_PIN_BTN_MODE_BIT);

    /* Buton UP: intrare + pull-up */
    GPIO_INPUT(NANO_PIN_BTN_UP_DDR,   NANO_PIN_BTN_UP_BIT);
    GPIO_PULLUP(NANO_PIN_BTN_UP_PORT, NANO_PIN_BTN_UP_BIT);

    /* Buton DOWN: intrare + pull-up */
    GPIO_INPUT(NANO_PIN_BTN_DN_DDR,   NANO_PIN_BTN_DN_BIT);
    GPIO_PULLUP(NANO_PIN_BTN_DN_PORT, NANO_PIN_BTN_DN_BIT);
}
