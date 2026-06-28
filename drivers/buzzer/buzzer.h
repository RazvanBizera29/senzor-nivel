#ifndef BUZZER_H
#define BUZZER_H

/*
 * buzzer.h — Driver buzzer activ, non-blocking
 * Foloseste millis() pentru timing fara delay().
 */

#include <stdint.h>

typedef enum {
    BUZZER_OFF      = 0,
    BUZZER_SLOW,        /* nivel ridicat: ON=200ms OFF=800ms */
    BUZZER_FAST,        /* nivel scazut:  ON=80ms  OFF=120ms */
} buzzer_mode_t;

void buzzer_init(void);
void buzzer_set_mode(buzzer_mode_t mode);
void buzzer_update(void);   /* apelat din main loop — non-blocking */

#endif /* BUZZER_H */
