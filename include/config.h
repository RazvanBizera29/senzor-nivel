#ifndef CONFIG_H
#define CONFIG_H

/*
 * config.h — Configuratie centralizata pini si parametri sistem
 * MCU: ATmega328P @ 16MHz
 */

#include <avr/io.h>

/* ===== CLOCK ===== */
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

/* ===== UART ===== */
#define UART_BAUD       9600UL
#define UART_UBRR       ((F_CPU / (16UL * UART_BAUD)) - 1)  /* = 103 */
#define UART_RX_BUFSIZE 64

/* ===== PINI GPIO ===== */

/* Releu pompa — PD7 (D7 pe Nano) */
#define RELAY_DDR       DDRD
#define RELAY_PORT      PORTD
#define RELAY_BIT       PD7

/* Buzzer activ — PD6 (D6 pe Nano) */
#define BUZZER_DDR      DDRD
#define BUZZER_PORT     PORTD
#define BUZZER_BIT      PD6

/* Buton MODE — PD2 (D2 = INT0) */
#define BTN_MODE_DDR    DDRD
#define BTN_MODE_PORT   PORTD
#define BTN_MODE_PIN    PIND
#define BTN_MODE_BIT    PD2

/* Buton UP — PD4 (D4) */
#define BTN_UP_DDR      DDRD
#define BTN_UP_PORT     PORTD
#define BTN_UP_PIN      PIND
#define BTN_UP_BIT      PD4

/* Buton DOWN — PD5 (D5) */
#define BTN_DOWN_DDR    DDRD
#define BTN_DOWN_PORT   PORTD
#define BTN_DOWN_PIN    PIND
#define BTN_DOWN_BIT    PD5

/* ===== ADC ===== */
#define ADC_CHANNEL     0       /* A0 = MUX3:0 = 0000 */
#define ADC_SAMPLES     8       /* Mediere pe 8 mostre */

/* Calibrare senzor HW-038 (modifica dupa masurare!) */
#define SENSOR_DRY      50
#define SENSOR_WET      900

/* ===== I2C / OLED ===== */
#define I2C_SCL_KHZ     400UL                              /* Fast-mode */
#define I2C_TWBR_VAL    ((F_CPU / (I2C_SCL_KHZ * 1000UL) - 16) / 2)  /* = 12 */
#define OLED_I2C_ADDR   0x3C

/* ===== CONTROL HISTEREZIS ===== */
#define SP_LOW_MIN      5
#define SP_LOW_MAX      90
#define SP_HIGH_MIN     10
#define SP_HIGH_MAX     95
#define SP_LOW_DEFAULT  25
#define SP_HIGH_DEFAULT 80
#define SP_GAP_MIN      10      /* Diferenta minima LOW-HIGH */

/* ===== ALERTE ===== */
#define ALERT_CRIT_LOW  8       /* % — buzzer + serial alert */
#define ALERT_CRIT_HIGH 95

/* ===== TIMING (ms) ===== */
#define T_SENSOR        500U
#define T_SERIAL        500U
#define T_DISPLAY       250U
#define T_DEBOUNCE      200U
#define T_BUZZER_ON_L   80U     /* Buzzer low-level alert: scurt */
#define T_BUZZER_OFF_L  120U
#define T_BUZZER_ON_H   200U    /* Buzzer high-level alert: lung */
#define T_BUZZER_OFF_H  800U

/* ===== MACROS UTILITARE ===== */
#define BIT_SET(reg, bit)    ((reg) |=  (1 << (bit)))
#define BIT_CLR(reg, bit)    ((reg) &= ~(1 << (bit)))
#define BIT_READ(reg, bit)   (((reg) >> (bit)) & 1)
#define BIT_TOGGLE(reg, bit) ((reg) ^=  (1 << (bit)))

#define CLAMP(x, lo, hi)     ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))

#endif /* CONFIG_H */
