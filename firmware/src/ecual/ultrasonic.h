/*
 * ultrasonic — HC-SR04 a interrupt, senza attese.
 *
 *   TRIG = D12 = PB4 (uscita)
 *   ECHO = D8  = PB0 (ingresso, interrupt PCINT0 su ogni fronte)
 *
 * Uso tipico, da un task ogni 60 ms:
 *   1. Ultrasonic_GetResult() → risultato della misura avviata 60 ms prima
 *   2. Ultrasonic_Trigger()   → avvia la misura successiva
 */
#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include <stdint.h>

typedef enum {
    US_OK = 0,      /* distanza valida in *mm */
    US_NO_ECHO,     /* nessun ostacolo entro la portata (o eco perso) */
    US_BUSY         /* misura ancora in corso */
} Us_Status;

/* Configura i pin e l'interrupt. */
void Ultrasonic_Init(void);

/* Avvia una misura: impulso di 10 µs su TRIG. */
void Ultrasonic_Trigger(void);

/* Risultato dell'ultima misura avviata. Se US_OK, scrive la distanza in *mm. */
Us_Status Ultrasonic_GetResult(uint16_t *mm);

#endif /* ULTRASONIC_H */
