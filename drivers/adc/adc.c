/*
 * adc.c — Driver ADC bare metal, ATmega328P
 * Registre: ADMUX, ADCSRA, ADCL, ADCH
 */

#include <avr/io.h>
#include "adc.h"
#include "bsp.h"

void adc_init(void)
{
    ADMUX  = NANO_ADC_REFS;                      /* AVcc, right-adjusted */
    ADCSRA = (1<<ADEN) | NANO_ADC_PRESCALER;    /* enable, prescaler 128 */

    /* Prima conversie (mai lenta — initializare interna) */
    ADCSRA |= (1<<ADSC);
    while (ADCSRA & (1<<ADSC));
}

uint16_t adc_read(uint8_t channel)
{
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);
    ADCSRA |= (1<<ADSC);
    while (ADCSRA & (1<<ADSC));
    /* ADCL trebuie citit primul — citire explicita */
    uint8_t lo = ADCL;
    uint8_t hi = ADCH;
    return ((uint16_t)hi << 8) | lo;
}

uint16_t adc_read_avg(uint8_t channel, uint8_t samples)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < samples; i++) {
        sum += adc_read(channel);
    }
    return (uint16_t)(sum / samples);
}
