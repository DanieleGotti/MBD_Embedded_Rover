/*
 * servo — implementazione.
 */
#include "servo.h"

#include "../mcal/timer1.h"

static int16_t s_angle = 0;
static int8_t  s_trim  = 0;   /* correzione del centro, in gradi */

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

    int32_t us = SERVO_CENTER_US + ((int32_t)s_angle + s_trim) * SERVO_US_PER_DEG;

    Timer1_SetCompareA((uint16_t)(us * TIMER1_TICKS_PER_US));
    Timer1_SetOutputA(true);   /* riattiva gli impulsi se il servo era rilasciato */
}

int16_t Servo_GetAngle(void)
{
    return s_angle;
}

void Servo_Release(void)
{
    /* Nessun impulso: alcuni servo interpretano anche un impulso
     * cortissimo come comando, quindi il pin va scollegato dal timer */
    Timer1_SetOutputA(false);
}

void Servo_SetTrim(int8_t deg)
{
    s_trim = deg;
    Servo_SetAngle(s_angle);   /* riapplica con il nuovo centro */
}
