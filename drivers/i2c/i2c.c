/*
 * i2c.c — Driver TWI master bare metal, ATmega328P
 *
 * Registre: TWBR, TWSR, TWCR, TWDR
 * SCL = F_CPU / (16 + 2*TWBR*prescaler)
 * 400kHz @ 16MHz: TWBR=12, prescaler=1
 */

#include <util/twi.h>
#include "i2c.h"
#include "bsp.h"

#define TWI_WAIT()  while (!(TWCR & (1<<TWINT)))

void i2c_init(void)
{
    TWSR = 0x00;                    /* prescaler = 1 */
    TWBR = (uint8_t)NANO_I2C_TWBR; /* 12 pentru 400kHz */
    TWCR = (1<<TWEN);
}

uint8_t i2c_start(uint8_t addr_rw)
{
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
    TWI_WAIT();
    uint8_t st = TWSR & 0xF8;
    if (st != TW_START && st != TW_REP_START) return 1;

    TWDR = addr_rw;
    TWCR = (1<<TWINT) | (1<<TWEN);
    TWI_WAIT();
    st = TWSR & 0xF8;
    if (st != TW_MT_SLA_ACK && st != TW_MR_SLA_ACK) return 1;
    return 0;
}

uint8_t i2c_write(uint8_t data)
{
    TWDR = data;
    TWCR = (1<<TWINT) | (1<<TWEN);
    TWI_WAIT();
    return ((TWSR & 0xF8) != TW_MT_DATA_ACK) ? 1 : 0;
}

uint8_t i2c_read_ack(void)
{
    TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
    TWI_WAIT();
    return TWDR;
}

uint8_t i2c_read_nack(void)
{
    TWCR = (1<<TWINT) | (1<<TWEN);
    TWI_WAIT();
    return TWDR;
}

void i2c_stop(void)
{
    TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN);
}

uint8_t i2c_write_buf(uint8_t addr, const uint8_t *buf, uint8_t len)
{
    if (i2c_start((uint8_t)((addr << 1) | TW_WRITE))) return 1;
    for (uint8_t i = 0; i < len; i++) {
        if (i2c_write(buf[i])) { i2c_stop(); return 1; }
    }
    i2c_stop();
    return 0;
}
