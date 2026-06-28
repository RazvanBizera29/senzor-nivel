#ifndef ADC_H
#define ADC_H

/*
 * adc.h — Driver ADC bare metal, ATmega328P
 * Referinta: AVcc | Prescaler: 128 (125kHz @ 16MHz)
 */

#include <stdint.h>

void     adc_init(void);
uint16_t adc_read(uint8_t channel);
uint16_t adc_read_avg(uint8_t channel, uint8_t samples);

#endif /* ADC_H */
