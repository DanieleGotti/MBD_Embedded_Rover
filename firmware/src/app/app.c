/*
 * app — implementazione.
 */
#include "app.h"

void App_Init(void)
{
}

void App_Step(const Rte_Commands *cmd, const Rte_Sensors *sen, Rte_Actuators *act)
{
    /* Servo: passa il comando così com'è */
    act->servo_deg = cmd->servo_deg;
    act->servo_on  = cmd->servo_on;

    /* PWM richiesti: direzione × velocità */
    act->pwm_l = (int16_t)cmd->drive_l * cmd->speed;
    act->pwm_r = (int16_t)cmd->drive_r * cmd->speed;

    /* Sicurezza: senza pile AA, o senza comandi recenti, i motori restano fermi */
    if (!sen->ts_present || !cmd->link_ok) {
        act->pwm_l = 0;
        act->pwm_r = 0;
    }

    /* Frenata automatica: in marcia avanti, con qualunque angolo del servo,
     * se l'ostacolo misurato è valido e vicino. Indietro e rotazioni restano permessi. */
    if (act->pwm_l > 0 && act->pwm_r > 0 &&
        sen->dist_valid && sen->dist_mm < APP_AEB_DIST_MM) {
        act->pwm_l = 0;
        act->pwm_r = 0;
        act->aeb_active = true;
    } else {
        act->aeb_active = false;
    }
}
