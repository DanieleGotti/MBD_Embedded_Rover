/*
 * motor — implementazione.
 */
#include "motor.h"

#include <avr/io.h>

#include "../mcal/pwm.h"

/* Satura v in −MOTOR_MAX … +MOTOR_MAX */
static int16_t Clamp(int16_t v)
{
    if (v > MOTOR_MAX) {
        return MOTOR_MAX;
    }
    if (v < -MOTOR_MAX) {
        return -MOTOR_MAX;
    }
    return v;
}

/* Valore assoluto, già nel range 0 … 255 */
static uint8_t Magnitude(int16_t v)
{
    return (uint8_t)((v < 0) ? -v : v);
}

void Motor_Init(void)
{
    PORTD &= ~((1 << PD7) | (1 << PD4));
    PORTB &= ~((1 << PB2) | (1 << PB3));
    DDRD  |=  (1 << PD7) | (1 << PD4);
    DDRB  |=  (1 << PB2) | (1 << PB3);
    Pwm_Init();
}

void Motor_Set(int16_t left, int16_t right)
{
    left  = Clamp(left);
    right = Clamp(right);

    if (left > 0) {
        PORTD |=  (1 << PD7);
        PORTD &= ~(1 << PD4);
    } else if (left < 0) {
        PORTD &= ~(1 << PD7);
        PORTD |=  (1 << PD4);
    } else {
        PORTD &= ~(1 << PD7);
        PORTD &= ~(1 << PD4);
    }
    Pwm_SetA(Magnitude(left));

    if (right > 0) {
        PORTB |=  (1 << PB2);
        PORTB &= ~(1 << PB3);
    } else if (right < 0) {
        PORTB &= ~(1 << PB2);
        PORTB |=  (1 << PB3);
    } else {
        PORTB &= ~(1 << PB2);
        PORTB &= ~(1 << PB3);
    }
    Pwm_SetB(Magnitude(right));
}

void Motor_Stop(void)
{
    Motor_Set(0, 0);
}
