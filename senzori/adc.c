/*
 * adc.c — Driver ADC bare metal, ATmega328P
 *
 * Registre ADC:
 *   ADMUX  — referinta (REFS1:0) + canal (MUX3:0) + aliniere (ADLAR)
 *   ADCSRA — ADEN (enable), ADSC (start), ADIF (done), ADPS2:0 (prescaler)
 *   ADC    — rezultat 10-bit (ADCL first, then ADCH)
 */

#include <avr/io.h>
#include "adc.h"

void adc_init(void)
{
    /*
     * REFS1=0, REFS0=1 → referinta AVcc (5V)
     * ADLAR=0 → rezultat right-adjusted (ADC = ADCL | (ADCH << 8))
     * MUX3:0 = 0000 → canal 0 (default, schimbat la fiecare citire)
     */
    ADMUX = (1 << REFS0);

    /*
     * ADEN=1  → ADC enabled
     * ADPS2:0 = 111 → prescaler 128 → 16MHz/128 = 125kHz
     */
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    /* Prima conversie este mai lenta (initializare interna) — o facem acum */
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
}

uint16_t adc_read(uint8_t channel)
{
    /* Seteaza canalul (MUX3:0), pastreaza referinta */
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

    /* Start Single Conversion */
    ADCSRA |= (1 << ADSC);

    /* Asteapta terminarea conversiei (ADSC revine la 0) */
    while (ADCSRA & (1 << ADSC));

    /*
     * Citeste rezultatul: ADCL trebuie citit primul!
     * Compilatorul C poate reordona — citire explicita in ordinea corecta.
     */
    uint8_t lo = ADCL;
    uint8_t hi = ADCH;
    return ((uint16_t)hi << 8) | lo;
}

uint16_t adc_read_avg(uint8_t ch, uint8_t n)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < n; i++) {
        sum += adc_read(ch);
    }
    return (uint16_t)(sum / n);
}
