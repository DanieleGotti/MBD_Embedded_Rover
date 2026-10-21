/*
 * pwm — Timer0 in PWM phase-correct, ≈ 3,9 kHz, due canali.
 *
 *   canale A: OC0A = PD6 = D6 → L293D pin 1 (enable motore sinistro)
 *   canale B: OC0B = PD5 = D5 → L293D pin 9 (enable motore destro)
 *
 * Duty 0 = sempre spento, 255 = sempre acceso.
 */
#ifndef PWM_H
#define PWM_H

#include <stdint.h>

/* Configura Timer0 e i due pin come uscite. Entrambi i duty partono da 0. */
void Pwm_Init(void);

void Pwm_SetA(uint8_t duty);
void Pwm_SetB(uint8_t duty);

#endif /* PWM_H */
