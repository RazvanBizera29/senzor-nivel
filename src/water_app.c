/*
 * water_app.c — Logica aplicatiei de monitorizare nivel apa
 *
 * Responsabilitati:
 *  - Citire senzor HW-038, conversie in procente
 *  - Control pompa cu histerezis
 *  - Update display OLED
 *  - Trimitere JSON pe serial
 *  - Receptie comenzi serial de la PC
 *  - Gestionare 3 butoane cu debounce + hold (driver button.h)
 *  - Gestionare buzzer (alerta)
 *  - Persistenta setpoint-uri in EEPROM
 *
 * Butoane:
 *  D2 (MODE)  — PRESSED: toggle AUTO/MANUAL
 *  D4 (FILL)  — PRESSED/HELD: porneste pompa umplere pana la sp_high
 *  D5 (DRAIN) — HELD: porneste pompa golire; RELEASED: opreste
 */

#include <string.h>
#include <stdlib.h>

#include "water_app.h"
#include "app_config.h"
#include "bsp.h"
#include "utils.h"
#include "button.h"

/* Drivere */
#include "adc.h"
#include "uart.h"
#include "timer.h"
#include "gpio.h"
#include "buzzer.h"
#include "ssd1306.h"
#include "eeprom.h"

/* ================================================================
 * STARE APLICATIE
 * ================================================================ */
static op_mode_t  mode       = OP_MODE_AUTO;
static uint8_t    pump_on    = 0;   /* pompa umplere */
static uint8_t    drain_on   = 0;   /* pompa golire  */
static int16_t    level_x10  = 0;   /* nivel% × 10, ex: 453 = 45.3% */
static uint8_t    level_pct  = 0;   /* nivel rotunjit 0-100 */
static uint16_t   adc_raw    = 0;

static int16_t    sp_low     = SP_LOW_DEFAULT;
static int16_t    sp_high    = SP_HIGH_DEFAULT;

/* Butoane */
static btn_t btn_mode;   /* D2 — toggle AUTO/MANUAL */
static btn_t btn_fill;   /* D4 — umplere rezervor   */
static btn_t btn_drain;  /* D5 — golire rezervor    */

/* Timing */
static uint32_t t_sensor  = 0;
static uint32_t t_serial  = 0;
static uint32_t t_display = 0;

/* Display dirty flags */
static uint8_t   prev_level = 0xFF;
static uint8_t   prev_pump  = 0xFF;
static op_mode_t prev_mode  = (op_mode_t)0xFF;

/* ================================================================
 * CONVERSIE ADC → PROCENTE
 * ================================================================ */
static int16_t adc_to_level(uint16_t raw)
{
    int32_t span = (int32_t)SENSOR_WET - (int32_t)SENSOR_DRY;
    if (span <= 0) return 0;
    int32_t v = ((int32_t)raw - (int32_t)SENSOR_DRY) * 1000L / span;
    return (int16_t)CLAMP(v, 0, 1000);
}

/* ================================================================
 * CONTROL LED-URI STATUS
 *  nivel <  20% → Rosu  (CRIT_LOW)
 *  nivel >= 80% → Verde (aproape plin)
 *  altfel        → Galben (nivel mediu)
 * Un singur LED activ la un moment dat.
 * ================================================================ */
static void leds_update(void)
{
    GPIO_LOW(NANO_PIN_LED_R_PORT, NANO_PIN_LED_R_BIT);
    GPIO_LOW(NANO_PIN_LED_Y_PORT, NANO_PIN_LED_Y_BIT);
    GPIO_LOW(NANO_PIN_LED_G_PORT, NANO_PIN_LED_G_BIT);

    if (level_pct < 20)
        GPIO_HIGH(NANO_PIN_LED_R_PORT, NANO_PIN_LED_R_BIT);
    else if (level_pct >= 80)
        GPIO_HIGH(NANO_PIN_LED_G_PORT, NANO_PIN_LED_G_BIT);
    else
        GPIO_HIGH(NANO_PIN_LED_Y_PORT, NANO_PIN_LED_Y_BIT);
}

/* ================================================================
 * CONTROL POMPA — HISTEREZIS
 * ================================================================ */
static void pump_control(void)
{
    if (mode == OP_MODE_MANUAL) return;

    if (!pump_on && level_pct <= (uint8_t)sp_low) {
        pump_on = 1;
        GPIO_HIGH(NANO_PIN_RELAY_PORT, NANO_PIN_RELAY_BIT);
    } else if (pump_on && level_pct >= (uint8_t)sp_high) {
        pump_on = 0;
        GPIO_LOW(NANO_PIN_RELAY_PORT, NANO_PIN_RELAY_BIT);
    }
}

/* ================================================================
 * HELPER — opreste ambele pompe
 * ================================================================ */
static void pumps_off(void)
{
    pump_on  = 0; GPIO_LOW(NANO_PIN_RELAY_PORT, NANO_PIN_RELAY_BIT);
    drain_on = 0; GPIO_LOW(NANO_PIN_DRAIN_PORT, NANO_PIN_DRAIN_BIT);
}

/* ================================================================
 * CITIRE BUTOANE (polling cu driver debounce + hold)
 *
 *  BTN_MODE  (D2): PRESSED → toggle AUTO/MANUAL
 *  BTN_FILL  (D4): PRESSED/HELD → porneste umplere pana la sp_high
 *                  (se opreste singur la sp_high sau la release)
 *  BTN_DRAIN (D5): HELD → pompa golire ON
 *                  RELEASED → pompa golire OFF
 * ================================================================ */
static void buttons_poll(uint32_t now)
{
    btn_event_t ev;

    /* --- MODE --- */
    ev = btn_update(&btn_mode, now);
    if (ev == BTN_PRESSED) {
        if (mode == OP_MODE_AUTO) {
            mode = OP_MODE_MANUAL;
            pumps_off();
        } else {
            mode = OP_MODE_AUTO;
        }
    }

    /* --- FILL --- */
    ev = btn_update(&btn_fill, now);
    if (ev == BTN_PRESSED || ev == BTN_HELD) {
        /* Porneste umplerea daca nivelul e sub sp_high */
        if (level_pct < (uint8_t)sp_high) {
            drain_on = 0; GPIO_LOW(NANO_PIN_DRAIN_PORT, NANO_PIN_DRAIN_BIT);
            pump_on  = 1; GPIO_HIGH(NANO_PIN_RELAY_PORT, NANO_PIN_RELAY_BIT);
        } else {
            /* Deja plin — opreste */
            pump_on = 0; GPIO_LOW(NANO_PIN_RELAY_PORT, NANO_PIN_RELAY_BIT);
        }
    } else if (ev == BTN_RELEASED) {
        /* In modul MANUAL oprim pompa la release */
        if (mode == OP_MODE_MANUAL) {
            pump_on = 0; GPIO_LOW(NANO_PIN_RELAY_PORT, NANO_PIN_RELAY_BIT);
        }
        /* In AUTO pompa continua prin histerezis */
    }

    /* --- DRAIN --- */
    ev = btn_update(&btn_drain, now);
    if (ev == BTN_HELD) {
        /* Golire: oprim umplerea si pornim golirea */
        pump_on  = 0; GPIO_LOW(NANO_PIN_RELAY_PORT, NANO_PIN_RELAY_BIT);
        drain_on = 1; GPIO_HIGH(NANO_PIN_DRAIN_PORT, NANO_PIN_DRAIN_BIT);
    } else if (ev == BTN_RELEASED) {
        drain_on = 0; GPIO_LOW(NANO_PIN_DRAIN_PORT, NANO_PIN_DRAIN_BIT);
    }
}

/* ================================================================
 * SERIAL — TRIMITE JSON
 * {"level":45.3,"pump":true,"mode":"AUTO","sp_low":25,"sp_high":80,"raw":512}
 * ================================================================ */
static void serial_send(void)
{
    uart_puts("{");
    uart_puts("\"level\":"); uart_put_float1(level_x10);
    uart_puts(",\"pump\":"); uart_puts(pump_on ? "true" : "false");
    uart_puts(",\"mode\":\""); uart_puts(mode == OP_MODE_AUTO ? "AUTO" : "MANUAL"); uart_puts("\"");
    uart_puts(",\"sp_low\":"); uart_put_int16(sp_low);
    uart_puts(",\"sp_high\":"); uart_put_int16(sp_high);
    uart_puts(",\"raw\":"); uart_put_uint16(adc_raw);
    uart_puts(",\"alert\":\"");
    if (level_pct <= ALERT_LEVEL_LOW)       uart_puts("CRIT_LOW");
    else if (level_pct >= ALERT_LEVEL_HIGH) uart_puts("CRIT_HIGH");
    else                                    uart_puts("OK");
    uart_puts("\",\"led\":\"");
    if (level_pct < 20)        uart_puts("RED");
    else if (level_pct >= 80)  uart_puts("GREEN");
    else                       uart_puts("YELLOW");
    uart_puts("\"}\r\n");
}

/* ================================================================
 * SERIAL — RECEPTIE COMENZI
 * PUMP_ON | PUMP_OFF | MODE_AUTO | MODE_MANUAL | SP_LOW:XX | SP_HIGH:XX
 * ================================================================ */
static void serial_receive(void)
{
    static char  buf[32];
    static uint8_t idx = 0;

    while (uart_available()) {
        char c = uart_getchar();
        if (c == '\r') continue;
        if (c == '\n') {
            buf[idx] = '\0';

            if (!strcmp(buf, "PUMP_ON")) {
                pump_on = 1;
                GPIO_HIGH(NANO_PIN_RELAY_PORT, NANO_PIN_RELAY_BIT);
            } else if (!strcmp(buf, "PUMP_OFF")) {
                pump_on = 0;
                GPIO_LOW(NANO_PIN_RELAY_PORT, NANO_PIN_RELAY_BIT);
            } else if (!strcmp(buf, "MODE_AUTO")) {
                mode = OP_MODE_AUTO;
            } else if (!strcmp(buf, "MODE_MANUAL")) {
                mode = OP_MODE_MANUAL;
                pump_on = 0;
                GPIO_LOW(NANO_PIN_RELAY_PORT, NANO_PIN_RELAY_BIT);
            } else if (!strncmp(buf, "SP_LOW:", 7)) {
                sp_low = CLAMP((int16_t)atoi(buf + 7), SP_LOW_MIN, sp_high - SP_GAP_MIN);
                eeprom_save_setpoints(sp_low, sp_high);
            } else if (!strncmp(buf, "SP_HIGH:", 8)) {
                sp_high = CLAMP((int16_t)atoi(buf + 8), sp_low + SP_GAP_MIN, SP_HIGH_MAX);
                eeprom_save_setpoints(sp_low, sp_high);
            }
            idx = 0;
        } else if (idx < 31) {
            buf[idx++] = c;
        }
    }
}

/* ================================================================
 * DISPLAY — UPDATE OLED
 * ================================================================ */
static void display_update(void)
{
    /* Page 0: Titlu + mod (doar la schimbare) */
    if (prev_mode != mode) {
        ssd1306_clear_page(0);
        ssd1306_puts_at(0, 0, "NIVEL APA");
        ssd1306_puts_at(0, 72, mode == OP_MODE_AUTO ? "[AUTO] " : "[MANUAL]");
        prev_mode = mode;
    }

    /* Page 1-2: Procent 2x (doar la schimbare) */
    if (prev_level != level_pct) {
        ssd1306_clear_page(1);
        ssd1306_clear_page(2);

        char buf[5];
        uint8_t i = 0;
        uint8_t n = level_pct;
        buf[i++] = (n >= 100) ? (char)('0' + n / 100) : ' ';
        n %= 100;
        buf[i++] = (level_pct >= 10) ? (char)('0' + n / 10) : ' ';
        buf[i++] = (char)('0' + level_pct % 10);
        buf[i++] = '%';
        buf[i]   = '\0';
        ssd1306_puts_2x(1, 0, buf);

        prev_level = level_pct;
    }

    /* Page 3: Pompa + alert (doar la schimbare) */
    if (prev_pump != pump_on) {
        ssd1306_clear_page(3);
        ssd1306_puts_at(3, 0, "POMPA:");
        ssd1306_puts_at(3, 42, pump_on ? " ON " : " OFF");

        if (level_pct <= ALERT_LEVEL_LOW)
            ssd1306_puts_at(3, 84, "!LOW!");
        else if (level_pct >= ALERT_LEVEL_HIGH)
            ssd1306_puts_at(3, 80, "!HIGH!");
        else
            ssd1306_puts_at(3, 86, "OK");

        prev_pump = pump_on;
    }

    /* Page 4: Bara nivel (mereu) */
    {
        uint8_t bar_w = (uint8_t)((uint16_t)level_pct * 128 / 100);
        ssd1306_draw_hbar(4, bar_w);
        ssd1306_draw_vline(4, (uint8_t)((uint16_t)sp_low  * 128 / 100));
        ssd1306_draw_vline(4, (uint8_t)((uint16_t)sp_high * 128 / 100));
    }

    /* Page 5: Setpoint */
    {
        ssd1306_clear_page(5);
        char sbuf[12];
        uint8_t i = 0;
        sbuf[i++] = 'S'; sbuf[i++] = 'P'; sbuf[i++] = ':';
        int16_t v = sp_low;
        if (v >= 100) sbuf[i++] = (char)('0' + v / 100);
        sbuf[i++] = (char)('0' + (v / 10) % 10);
        sbuf[i++] = (char)('0' + v % 10);
        sbuf[i++] = '-';
        v = sp_high;
        if (v >= 100) sbuf[i++] = (char)('0' + v / 100);
        sbuf[i++] = (char)('0' + (v / 10) % 10);
        sbuf[i++] = (char)('0' + v % 10);
        sbuf[i++] = '%';
        sbuf[i]   = '\0';
        ssd1306_puts_at(5, 0, sbuf);
    }
}

/* ================================================================
 * INIT
 * ================================================================ */
void water_app_init(void)
{
    /* Incarca setpoint-urile salvate din EEPROM */
    eeprom_load_setpoints(&sp_low, &sp_high);

    /* Init butoane — debounce 50ms, hold 600ms, repeat 150ms */
    btn_init(&btn_mode,  &NANO_PIN_BTN_MODE_PIN, NANO_PIN_BTN_MODE_BIT);
    btn_init(&btn_fill,  &NANO_PIN_BTN_UP_PIN,   NANO_PIN_BTN_UP_BIT);
    btn_init(&btn_drain, &NANO_PIN_BTN_DN_PIN,   NANO_PIN_BTN_DN_BIT);

    /* Ecran de bun venit */
    ssd1306_puts_at(2, 10, "Water Level v2.0");
    ssd1306_puts_at(4, 25, "Bare Metal AVR");
}

/* ================================================================
 * LOOP PRINCIPAL
 * ================================================================ */
void water_app_run(void)
{
    uint32_t now = millis();

    /* Butoane — debounce + hold (apelat cat mai des, fara blocare) */
    buttons_poll(now);

    /* Citire senzor */
    if (ELAPSED(now, t_sensor, T_SENSOR_MS)) {
        adc_raw   = adc_read_avg(NANO_PIN_SENSOR_CH, SENSOR_SAMPLES);
        level_x10 = adc_to_level(adc_raw);
        level_pct = (uint8_t)(level_x10 / 10);

        /* Alerta buzzer */
        if (level_pct <= ALERT_LEVEL_LOW)
            buzzer_set_mode(BUZZER_FAST);
        else if (level_pct >= ALERT_LEVEL_HIGH)
            buzzer_set_mode(BUZZER_SLOW);
        else
            buzzer_set_mode(BUZZER_OFF);

        pump_control();
        leds_update();
        t_sensor = now;
    }

    /* Buzzer non-blocking */
    buzzer_update();

    /* Display */
    if (ELAPSED(now, t_display, T_DISPLAY_MS)) {
        display_update();
        t_display = now;
    }

    /* Serial send */
    if (ELAPSED(now, t_serial, T_SERIAL_MS)) {
        serial_send();
        t_serial = now;
    }

    /* Serial receive */
    serial_receive();
}
