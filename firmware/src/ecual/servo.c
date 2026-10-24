/*
 * servo — implementazione.
 */
#include "servo.h"

#include "../mcal/timer1.h"

static int16_t s_angle = 0;

void Servo_Init(void)
{
    Timer1_Init();
    Servo_SetAngle(0);
}

void Servo_SetAngle(int16_t deg)
{
    if (deg > SERVO_MAX_DEG) {
        s_angle = SERVO_MAX_DEG;
    } else if (deg < SERVO_MIN_DEG) {
        s_angle = SERVO_MIN_DEG;
    } else {
        s_angle = deg;
    }

    int32_t us = SERVO_CENTER_US + (int32_t)s_angle * SERVO_US_PER_DEG;

    Timer1_SetCompareA((uint16_t)(us * TIMER1_TICKS_PER_US));
}

int16_t Servo_GetAngle(void)
{
    return s_angle;
}

void Servo_Release(void)
{
    /* OCR1A = 0: l'uscita resta bassa per tutto il periodo (resta un
     * impulso di 0,5 µs, troppo corto perché il servo lo consideri) */
    Timer1_SetCompareA(0);
}
