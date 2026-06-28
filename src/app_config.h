#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/*
 * app_config.h — Configuratie nivel aplicatie
 * Modifica valorile de aici pentru a ajusta comportamentul sistemului.
 * Pinii si hardware-ul sunt in bsp/nano.h
 */

/* ---- Calibrare senzor HW-038 ---- */
/* Masoara valorile ADC cu senzorul uscat si scufundat, pune-le aici */
#define SENSOR_DRY      50      /* ADC cand senzorul e uscat       */
#define SENSOR_WET      900     /* ADC cand senzorul e in apa      */
#define SENSOR_SAMPLES  8       /* Numar mostre pentru mediere     */

/* ---- Setpoint histerezis (valori implicite) ---- */
#define SP_LOW_DEFAULT  25      /* % — pornire pompa               */
#define SP_HIGH_DEFAULT 80      /* % — oprire pompa                */
#define SP_LOW_MIN      5
#define SP_LOW_MAX      90
#define SP_HIGH_MIN     10
#define SP_HIGH_MAX     95
#define SP_GAP_MIN      10      /* Diferenta minima LOW < HIGH     */

/* ---- Alerte ---- */
#define ALERT_LEVEL_LOW  8      /* % — BUZZER_FAST (nivel critic jos)  */
#define ALERT_LEVEL_HIGH 95     /* % — BUZZER_SLOW (nivel critic sus)  */

/* ---- Timing (milisecunde) ---- */
#define T_SENSOR_MS     500U    /* Interval citire senzor          */
#define T_SERIAL_MS     500U    /* Interval trimitere JSON serial  */
#define T_DISPLAY_MS    250U    /* Interval refresh display        */
#define T_DEBOUNCE_MS   200U    /* Debounce butoane                */

#endif /* APP_CONFIG_H */
