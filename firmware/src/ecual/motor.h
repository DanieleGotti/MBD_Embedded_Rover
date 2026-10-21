/*
 * motor — i due motori tramite il L293D.
 *
 *            enable (PWM)     IN a           IN b
 *   sinistro D6 (OC0A)        D7 = PD7 (IN1) D4 = PD4 (IN2)
 *   destro   D5 (OC0B)        D10 = PB2 (IN3) D11 = PB3 (IN4)
 *
 * Verso: IN a = 1, IN b = 0 → avanti; IN a = 0, IN b = 1 → indietro;
 *        IN a = IN b → motore fermo (frenato).
 */
#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

#define MOTOR_MAX 255

/*
 * Porta i pin in uno stato sicuro (motori fermi) e configura il PWM.
 * Va chiamata per prima in main(), prima di qualunque altro modulo.
 */
void Motor_Init(void);

/*
 * Comando dei due motori: −255 … +255.
 * Positivo = avanti, negativo = indietro, 0 = fermo.
 * Valori fuori range vengono saturati.
 */
void Motor_Set(int16_t left, int16_t right);

/* Equivale a Motor_Set(0, 0). */
void Motor_Stop(void);

#endif /* MOTOR_H */
