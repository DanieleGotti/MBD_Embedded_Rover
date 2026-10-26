/*
 * servo — micro servo su D9, comandato in gradi.
 *
 * Il servo legge la durata di un impulso ripetuto ogni 20 ms:
 *   ≈ 1500 µs → centro (0°), più corto → un verso, più lungo → l'altro.
 * Le costanti sotto sono valori tipici: vanno calibrate sul servo reale
 * (diventeranno CAL_SERVO_ZERO e CAL_SERVO_GAIN).
 */
#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>

#define SERVO_CENTER_US   1500     /* impulso a 0° */
#define SERVO_US_PER_DEG  11       /* ≈ (2500 − 500) µs / 180° */
#define SERVO_MIN_DEG     (-40)    /* limiti di sicurezza: lontani dai fine corsa */
#define SERVO_MAX_DEG     40

/* Configura il timer e porta il servo al centro. */
void Servo_Init(void);

/* Angolo in gradi, saturato in SERVO_MIN_DEG … SERVO_MAX_DEG. 0 = dritto avanti. */
void Servo_SetAngle(int16_t deg);

/* Ultimo angolo comandato (già saturato). */
int16_t Servo_GetAngle(void);

/*
 * Smette di mandare impulsi: il servo non tiene più la posizione con forza,
 * ma non ronza e non consuma. Il prossimo Servo_SetAngle lo riattiva.
 */
void Servo_Release(void);

/*
 * Correzione del centro, in gradi (calibrazione CAL_SERVO_ZERO):
 * compensa il montaggio non perfettamente dritto dell'ultrasuono.
 */
void Servo_SetTrim(int8_t deg);

#endif /* SERVO_H */
