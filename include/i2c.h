#ifndef I2C_H
#define I2C_H

/*
 * i2c.h — Driver TWI (I2C) bare metal, ATmega328P
 * Modul master, polling (fara intreruperi)
 *
 * SDA = PC4 (A4 pe Nano)
 * SCL = PC5 (A5 pe Nano)
 */

#include <stdint.h>

/* Returneaza 0 = succes, 1 = eroare */
void    i2c_init(void);
uint8_t i2c_start(uint8_t addr_rw);    /* addr_rw = (addr << 1) | 0/1 */
uint8_t i2c_write(uint8_t data);
uint8_t i2c_read_ack(void);
uint8_t i2c_read_nack(void);
void    i2c_stop(void);

/* Shorthand: scrie N bytes catre addr fara a gestiona start/stop manual */
uint8_t i2c_write_buf(uint8_t addr, const uint8_t *buf, uint8_t len);

#endif /* I2C_H */
