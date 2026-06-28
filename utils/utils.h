#ifndef UTILS_H
#define UTILS_H

/*
 * utils.h — Macros utilitare generale
 */

#include <stdint.h>

/* Operatii pe biti */
#define BIT_SET(reg, bit)     ((reg) |=  (1U << (bit)))
#define BIT_CLR(reg, bit)     ((reg) &= ~(1U << (bit)))
#define BIT_READ(reg, bit)    (((reg) >> (bit)) & 1U)
#define BIT_TOGGLE(reg, bit)  ((reg) ^=  (1U << (bit)))

/* Limitare valoare */
#define CLAMP(x, lo, hi)  ((x) < (lo) ? (lo) : ((x) > (hi) ? (hi) : (x)))

/* Dimensiune array la compile-time */
#define ARRAY_SIZE(arr)   (sizeof(arr) / sizeof((arr)[0]))

/* Marcare variabila neutilizata (evita warning -Wunused) */
#define UNUSED(x)         ((void)(x))

/* Timeout simplu bazat pe millis (non-blocking) */
#define ELAPSED(now, last, interval) \
    ((uint32_t)((now) - (last)) >= (uint32_t)(interval))

#endif /* UTILS_H */
