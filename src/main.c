#include <avr/interrupt.h>
#include "timer.h"
#include "uart.h"
#include "adc.h"
#include "gpio.h"
#include "buzzer.h"
#include "ssd1306.h"
#include "delay.h"
#include "water_app.h"

int main(void)
{
    /* 1. Initializare periferice hardware */
    gpio_init();
    timer_init();
    uart_init();
    adc_init();
    buzzer_init();

    /* 2. Activeaza intreruperile globale */
    sei();

    /* 3. Periferice care necesita intreruperi active */
    ssd1306_init();   /* nu bloca daca OLED lipseste — I2C are timeout */

    /* 4. Init aplicatie + ecran de bun venit */
    water_app_init();
    delay_ms(1500);
    ssd1306_clear();

    uart_puts("{\"status\":\"ready\"}\r\n");

    /* 5. Loop infinit */
    for (;;) {
        water_app_run();
    }

    return 0;
}
