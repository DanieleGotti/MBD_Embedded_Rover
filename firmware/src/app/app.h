/*
 * app — logica applicativa (ASW), scritta a mano.
 *
 * È una funzione "pura": legge solo i suoi ingressi, scrive solo le sue
 * uscite, non tocca registri né driver. Per questo potrà essere
 * sostituita, con la stessa interfaccia, dal codice generato da Simulink.
 */
#ifndef APP_H
#define APP_H

#include "../rte/rte.h"

#define APP_PERIOD_MS   20u
#define APP_AEB_DIST_MM 100u   /* frenata automatica sotto questa distanza */

void App_Init(void);

/* Un passo di calcolo, ogni APP_PERIOD_MS */
void App_Step(const Rte_Commands *cmd, const Rte_Sensors *sen, Rte_Actuators *act);

#endif /* APP_H */
