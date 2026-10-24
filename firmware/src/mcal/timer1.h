/*
 * timer1 — Timer1 a 16 bit in Fast PWM con periodo 20 ms.
 *
 *   modalità 14: Fast PWM, TOP = ICR1
 *   prescaler 8: 16 MHz / 8 = 2 MHz → 1 tick = 0,5 µs
 *   periodo = (ICR1 + 1) tick = 40000 × 0,5 µs = 20 ms
 *
 * Uscita OC1A = PB1 = D9 (servo). OC1B (D10) resta un normale GPIO.
 * Il contatore TCNT1 è usato anche come base dei tempi fine (0,5 µs)
 * per misurare l'eco dell'ultrasuono.
 */
#ifndef TIMER1_H
#define TIMER1_H

#include <stdint.h>

#define TIMER1_TICKS_PER_US 2u
#define TIMER1_TOP          39999u      /* 20 ms */

/* Configura Timer1 e D9 come uscita. Larghezza iniziale dell'impulso: 1,5 ms. */
void Timer1_Init(void);

/* Durata dell'impulso su D9, in tick da 0,5 µs (0 … TIMER1_TOP). */
void Timer1_SetCompareA(uint16_t ticks);

/* Valore corrente del contatore (0 … TIMER1_TOP), lettura atomica. */
uint16_t Timer1_GetCount(void);

#endif /* TIMER1_H */
