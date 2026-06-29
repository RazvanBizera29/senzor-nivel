/*
 * button.h — Driver generic butoane cu debounce + hold detection
 *
 * Stare masina:
 *   IDLE → (press detectat) → DEBOUNCE → (50ms ok) → DOWN → BTN_PRESSED
 *   DOWN → (tinut 600ms)   → HELD      → BTN_HELD (repeta la 150ms)
 *   DOWN/HELD → (eliberat) → IDLE      → BTN_RELEASED
 *
 * Butoane active LOW (pull-up intern, apasare = scurtcircuit la GND).
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include <avr/io.h>

/* ---- Timpi (ms) ---- */
#define BTN_DEBOUNCE_MS   50u   /* cat timp trebuie sa stea apasat pt confirmare */
#define BTN_HOLD_MS      600u   /* dupa cat timp se declanseaza modul HELD        */
#define BTN_HOLD_RPT_MS  150u   /* interval repetare event HELD (cat timp e tinut)*/

/* ---- Eveniment returnat de btn_update() ---- */
typedef enum {
    BTN_NONE     = 0,   /* nimic nou */
    BTN_PRESSED,        /* prima apasare confirmata (debounced) */
    BTN_HELD,           /* tinut apasat — se repeta la BTN_HOLD_RPT_MS */
    BTN_RELEASED,       /* tocmai eliberat */
} btn_event_t;

/* ---- Stare interna (nu modifica direct) ---- */
typedef enum {
    _BS_IDLE = 0,
    _BS_DEBOUNCE,
    _BS_DOWN,
    _BS_HELD,
} _btn_state_t;

typedef struct {
    volatile uint8_t *pin_reg;  /* registrul PIN al portului (ex: &PIND) */
    uint8_t           bit;      /* bitul pinului (ex: PD2) */
    _btn_state_t      state;
    uint32_t          t_edge;   /* timestamp la prima detectie apasare */
    uint32_t          t_rpt;    /* timestamp ultimului event HELD */
} btn_t;

/* ---- API ---- */
void        btn_init(btn_t *b, volatile uint8_t *pin_reg, uint8_t bit);
btn_event_t btn_update(btn_t *b, uint32_t now_ms);

#endif /* BUTTON_H */
