#ifndef ADC_H
#define ADC_H

/*
 * adc.h — Driver ADC bare metal, ATmega328P
 * Referinta: AVcc (5V)
 * Prescaler: 128 -> 16MHz/128 = 125kHz (recomandat 50-200kHz)
 */

#include <stdint.h>

void     adc_init(void);
uint16_t adc_read(uint8_t channel);         /* citire single shot */
uint16_t adc_read_avg(uint8_t ch, uint8_t n); /* media pe n mostre */

#endif /* ADC_H */
