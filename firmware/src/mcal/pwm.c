/*
 * pwm — implementazione su Timer0.
 *
 * Phase-correct (WGM0 = 1): il contatore sale 0→255 e scende 255→0.
 * Frequenza = F_CPU / (prescaler · 510) = 16 MHz / (8 · 510) ≈ 3,92 kHz
 */
#include "pwm.h"

#include <avr/io.h>

void Pwm_Init(void)
{
    OCR0A = 0;
    OCR0B = 0;
    DDRD |= (1 << PD5) | (1 << PD6);
    TCCR0A = (1 << COM0A1) | (1 << COM0B1) | (1 << WGM00);
    TCCR0B = (1 << CS01);
}

void Pwm_SetA(uint8_t duty)
{
    OCR0A = duty;
}

void Pwm_SetB(uint8_t duty)
{
    OCR0B = duty;
}
