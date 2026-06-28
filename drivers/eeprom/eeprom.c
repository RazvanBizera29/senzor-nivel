/*
 * eeprom.c — Driver EEPROM bare metal, ATmega328P
 *
 * Registre EEPROM:
 *   EEAR  (EEARH:EEARL) — adresa (10-bit pe 328P)
 *   EEDR  — data register
 *   EECR  — control: EERE (read enable), EEPE (write enable),
 *                    EEMPE (master write enable), EEPM1:0 (mode)
 *
 * Scriere: EEMPE=1 → EEPE=1 (in max 4 cicluri CPU dupa EEMPE)
 * Citire:  EERE=1  → EEDR disponibil in acelasi ciclu
 *
 * IMPORTANT: Intreruperile sunt dezactivate pe durata scrierii
 * pentru a respecta secventa critica de timp (datasheet p.21).
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "eeprom.h"
#include "app_config.h"

/* ================================================================
 * Citire byte
 * ================================================================ */
uint8_t eeprom_read_byte(uint16_t addr)
{
    /* Asteapta terminarea oricarei scrieri anterioare */
    while (EECR & (1 << EEPE));

    EEAR = addr;
    EECR |= (1 << EERE);   /* start read */
    return EEDR;
}

/* ================================================================
 * Scriere byte (cu dezactivare intreruperi pe secventa critica)
 * ================================================================ */
void eeprom_write_byte(uint16_t addr, uint8_t data)
{
    /* Asteapta terminarea scrierilor anterioare */
    while (EECR & (1 << EEPE));

    EEAR = addr;
    EEDR = data;

    /* Secventa critica: EEMPE, apoi EEPE in max 4 cicluri CPU */
    uint8_t sreg = SREG;
    __builtin_avr_cli();
    EECR |= (1 << EEMPE);
    EECR |= (1 << EEPE);
    SREG = sreg;    /* restaureaza I-flag */
}

/* ================================================================
 * Load setpoints din EEPROM
 * Daca magic byte lipseste (primul boot), incarca valorile default.
 * ================================================================ */
void eeprom_load_setpoints(int16_t *sp_low, int16_t *sp_high)
{
    if (eeprom_read_byte(EEPROM_ADDR_MAGIC) != EEPROM_MAGIC) {
        /* EEPROM neinitialized — foloseste valorile default */
        *sp_low  = SP_LOW_DEFAULT;
        *sp_high = SP_HIGH_DEFAULT;
        /* Scrie valorile default in EEPROM pentru data viitoare */
        eeprom_save_setpoints(*sp_low, *sp_high);
        return;
    }

    *sp_low  = (int16_t)eeprom_read_byte(EEPROM_ADDR_LOW);
    *sp_high = (int16_t)eeprom_read_byte(EEPROM_ADDR_HIGH);
}

/* ================================================================
 * Save setpoints in EEPROM
 * Scrie doar daca valoarea s-a schimbat (protejeaza ciclurile).
 * ================================================================ */
void eeprom_save_setpoints(int16_t sp_low, int16_t sp_high)
{
    /* Scrie magic byte */
    if (eeprom_read_byte(EEPROM_ADDR_MAGIC) != EEPROM_MAGIC)
        eeprom_write_byte(EEPROM_ADDR_MAGIC, EEPROM_MAGIC);

    /* Scrie doar daca s-a schimbat (economiseste cicluri EEPROM) */
    if (eeprom_read_byte(EEPROM_ADDR_LOW) != (uint8_t)sp_low)
        eeprom_write_byte(EEPROM_ADDR_LOW, (uint8_t)sp_low);

    if (eeprom_read_byte(EEPROM_ADDR_HIGH) != (uint8_t)sp_high)
        eeprom_write_byte(EEPROM_ADDR_HIGH, (uint8_t)sp_high);
}
