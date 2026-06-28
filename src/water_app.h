#ifndef WATER_APP_H
#define WATER_APP_H

/*
 * water_app.h — Logica aplicatiei de monitorizare nivel apa
 * Apelat din main.c
 */

#include <stdint.h>

typedef enum {
    OP_MODE_AUTO   = 0,
    OP_MODE_MANUAL = 1
} op_mode_t;

/* Init + loop */
void water_app_init(void);
void water_app_run(void);       /* apelat continuu din main loop */

/* Callback pentru butonul MODE (inregistrat la interrupt driver) */
void water_app_on_mode_btn(void);

#endif /* WATER_APP_H */
