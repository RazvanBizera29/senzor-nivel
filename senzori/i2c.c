/*
 * i2c.c — Driver TWI (I2C) bare metal, ATmega328P
 *
 * Registre TWI:
 *   TWBR  — Baud Rate Register  (SCL freq = F_CPU / (16 + 2*TWBR*prescaler))
 *   TWSR  — Status Register     (TWPS1:0 = prescaler bits)
 *   TWCR  — Control Register    (TWINT, TWEA, TWSTA, TWSTO, TWEN)
 *   TWDR  — Data Register
 *   TWAR  — Address Register    (master mode: neutilizat)
 *
 * Coduri status TWI (prescaler=1):
 *   0x08 — START transmis
 *   0x10 — Re-START transmis
 *   0x18 — SLA+W transmis, ACK primit
 *   0x20 — SLA+W transmis, NACK
 *   0x28 — Data transmisa, ACK primit
 *   0x30 — Data transmisa, NACK
 *   0x40 — SLA+R transmis, ACK primit
 *   0x50 — Data primita, ACK returnat
 *   0x58 — Data primita, NACK returnat
 */

#include <util/twi.h>
#include "i2c.h"
#include "config.h"

/* Asteapta terminarea operatiei TWI curente */
#define TWI_WAIT()  while (!(TWCR & (1 << TWINT)))

void i2c_init(void)
{
    /* Prescaler = 1 (TWPS1:0 = 00) */
    TWSR = 0x00;

    /*
     * SCL = F_CPU / (16 + 2 * TWBR * prescaler)
     * 400kHz: TWBR = (16000000 / 400000 - 16) / 2 = 12
     */
    TWBR = (uint8_t)I2C_TWBR_VAL;

    /* Activeaza TWI */
    TWCR = (1 << TWEN);
}

uint8_t i2c_start(uint8_t addr_rw)
{
    /* Trimite conditie START */
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    TWI_WAIT();

    if ((TWSR & 0xF8) != TW_START && (TWSR & 0xF8) != TW_REP_START)
        return 1;

    /* Trimite adresa + R/W */
    TWDR = addr_rw;
    TWCR = (1 << TWINT) | (1 << TWEN);
    TWI_WAIT();

    uint8_t status = TWSR & 0xF8;
    if (status != TW_MT_SLA_ACK && status != TW_MR_SLA_ACK)
        return 1;

    return 0;
}

uint8_t i2c_write(uint8_t data)
{
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    TWI_WAIT();

    if ((TWSR & 0xF8) != TW_MT_DATA_ACK)
        return 1;
    return 0;
}

uint8_t i2c_read_ack(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
    TWI_WAIT();
    return TWDR;
}

uint8_t i2c_read_nack(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN);
    TWI_WAIT();
    return TWDR;
}

void i2c_stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    /* TWINT nu e setat dupa STOP — nu asteaptam */
}

uint8_t i2c_write_buf(uint8_t addr, const uint8_t *buf, uint8_t len)
{
    if (i2c_start((uint8_t)(addr << 1) | TW_WRITE)) return 1;
    for (uint8_t i = 0; i < len; i++) {
        if (i2c_write(buf[i])) { i2c_stop(); return 1; }
    }
    i2c_stop();
    return 0;
}
