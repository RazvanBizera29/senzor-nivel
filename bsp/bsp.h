#ifndef BSP_H
#define BSP_H

/*
 * bsp.h — Board Support Package entry point
 *
 * Include acest fisier in drivere si aplicatie.
 * Schimba board-ul selectat mai jos daca portezi pe alt hardware.
 */

/* ---- Selectie board ---- */
#define BOARD_ARDUINO_NANO

#if defined(BOARD_ARDUINO_NANO)
#   include "nano.h"
#elif defined(BOARD_ARDUINO_UNO)
#   include "uno.h"
#else
#   error "Niciun board selectat in bsp.h"
#endif

#endif /* BSP_H */
