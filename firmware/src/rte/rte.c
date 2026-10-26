/*
 * rte — implementazione: traduce tra segnali e chiamate ai driver.
 */
#include "rte.h"

#include "../mcal/tick.h"
#include "../ecual/motor.h"
#include "../ecual/servo.h"
#include "../ecual/ultrasonic.h"
#include "../ecual/battery.h"

static Rte_Commands s_cmd = {
    .drive_l   = 0,
    .drive_r   = 0,
    .speed     = 200,
    .servo_deg = 0,
    .servo_on  = true,
    .link_ok   = false,
};

/* Ultima misura completata dell'ultrasuono */
static Us_Status s_us_status = US_BUSY;
static uint16_t  s_us_mm     = 0;

Rte_Commands *Rte_GetCommands(void)
{
    return &s_cmd;
}

void Rte_UltrasonicCycle(void)
{
    uint16_t  mm;
    Us_Status st = Ultrasonic_GetResult(&mm);

    /* "In corso" non sovrascrive l'ultima misura buona */
    if (st != US_BUSY) {
        s_us_status = st;
        s_us_mm     = mm;
    }
    Ultrasonic_Trigger();
}

void Rte_ReadSensors(Rte_Sensors *sen)
{
    sen->time_ms = Tick_GetMs();
    sen->vbat_mv = Battery_GetMv();
    sen->ts_present = Battery_IsPresent();
    sen->dist_valid = (s_us_status == US_OK);
    sen->dist_mm = sen->dist_valid ? s_us_mm : 0;
}

void Rte_WriteActuators(const Rte_Actuators *act)
{
    Motor_Set(act->pwm_l, act->pwm_r);
    if (act->servo_on) {
        Servo_SetAngle(act->servo_deg);
    } else {
        Servo_Release();
    }
}
