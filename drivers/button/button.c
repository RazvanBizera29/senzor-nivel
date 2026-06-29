/*
 * button.c — Driver generic butoane cu debounce + hold detection
 *
 * Apeleaza btn_update() din main loop cat mai des posibil (la fiecare ms
 * e ideal, dar functioneaza si la 5-10ms).
 * Butoane active LOW: apasare = 0 pe pin (pull-up intern activ).
 */

#include "button.h"

/* ================================================================
 * btn_init — Initializeaza structura unui buton
 * pin_reg: pointer la registrul PIN al portului (ex: &PIND)
 * bit:     bitul pinului (ex: PD2)
 * ================================================================ */
void btn_init(btn_t *b, volatile uint8_t *pin_reg, uint8_t bit)
{
    b->pin_reg = pin_reg;
    b->bit     = bit;
    b->state   = _BS_IDLE;
    b->t_edge  = 0;
    b->t_rpt   = 0;
}

/* ================================================================
 * btn_update — Masina de stare, apeleaza din loop cu millis() curent
 * Returneaza: BTN_PRESSED, BTN_HELD, BTN_RELEASED sau BTN_NONE
 * ================================================================ */
btn_event_t btn_update(btn_t *b, uint32_t now_ms)
{
    /* active LOW: 0 = apasat */
    uint8_t raw_pressed = !(*b->pin_reg & (1u << b->bit));

    switch (b->state) {

    case _BS_IDLE:
        if (raw_pressed) {
            b->state  = _BS_DEBOUNCE;
            b->t_edge = now_ms;
        }
        break;

    case _BS_DEBOUNCE:
        if (!raw_pressed) {
            /* s-a eliberat inainte de debounce — zgomot, ignora */
            b->state = _BS_IDLE;
        } else if ((now_ms - b->t_edge) >= BTN_DEBOUNCE_MS) {
            /* apasare confirmata */
            b->state = _BS_DOWN;
            return BTN_PRESSED;
        }
        break;

    case _BS_DOWN:
        if (!raw_pressed) {
            b->state = _BS_IDLE;
            return BTN_RELEASED;
        }
        if ((now_ms - b->t_edge) >= BTN_HOLD_MS) {
            b->state = _BS_HELD;
            b->t_rpt = now_ms;
            return BTN_HELD;   /* primul event held */
        }
        break;

    case _BS_HELD:
        if (!raw_pressed) {
            b->state = _BS_IDLE;
            return BTN_RELEASED;
        }
        if ((now_ms - b->t_rpt) >= BTN_HOLD_RPT_MS) {
            b->t_rpt = now_ms;
            return BTN_HELD;   /* repeat la 150ms cat timp e tinut */
        }
        break;
    }

    return BTN_NONE;
}
