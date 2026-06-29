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
void water_app_run(void);   /* apelat continuu din main loop */

#endif /* WATER_APP_H */
