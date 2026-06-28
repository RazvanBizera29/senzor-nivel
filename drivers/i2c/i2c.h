#ifndef I2C_H
#define I2C_H

/*
 * i2c.h — Driver TWI (I2C) master, bare metal, ATmega328P
 * SDA = PC4 (A4), SCL = PC5 (A5)
 * Mod: polling, fara intreruperi
 */

#include <stdint.h>

void    i2c_init(void);
uint8_t i2c_start(uint8_t addr_rw);        /* (addr<<1)|TW_WRITE/TW_READ */
uint8_t i2c_write(uint8_t data);           /* returneaza 0=ok, 1=eroare */
uint8_t i2c_read_ack(void);
uint8_t i2c_read_nack(void);
void    i2c_stop(void);
uint8_t i2c_write_buf(uint8_t addr, const uint8_t *buf, uint8_t len);

#endif /* I2C_H */
