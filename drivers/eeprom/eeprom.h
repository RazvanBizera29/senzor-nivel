#ifndef EEPROM_H
#define EEPROM_H

/*
 * eeprom.h — Driver EEPROM bare metal, ATmega328P
 *
 * Folosit pentru a persista setpoint-urile SP_LOW / SP_HIGH
 * intre reporniri. La primul boot (EEPROM gol), se incarca
 * valorile default din app_config.h.
 *
 * Layout EEPROM:
 *   Addr 0x00 — MAGIC byte (0xAB) — valideaza ca datele sunt scrise
 *   Addr 0x01 — sp_low  (uint8_t)
 *   Addr 0x02 — sp_high (uint8_t)
 *
 * ATmega328P: 1KB EEPROM, ~100.000 cicluri scriere per celula.
 */

#include <stdint.h>

#define EEPROM_MAGIC      0xAB
#define EEPROM_ADDR_MAGIC 0x00
#define EEPROM_ADDR_LOW   0x01
#define EEPROM_ADDR_HIGH  0x02

/* Citire / scriere byte individual */
uint8_t eeprom_read_byte(uint16_t addr);
void    eeprom_write_byte(uint16_t addr, uint8_t data);

/* Setpoint-uri aplicatie */
void    eeprom_load_setpoints(int16_t *sp_low, int16_t *sp_high);
void    eeprom_save_setpoints(int16_t sp_low, int16_t sp_high);

#endif /* EEPROM_H */
