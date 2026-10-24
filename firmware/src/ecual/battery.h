/*
 * battery — tensione delle pile AA (dominio TS) su A2.
 *
 * Partitore 10 kΩ / 10 kΩ: su A2 arriva VBAT / 2.
 * La misura viene filtrata con un passa-basso del primo ordine
 * per togliere il rumore dei motori.
 */
#ifndef BATTERY_H
#define BATTERY_H

#include <stdbool.h>
#include <stdint.h>

#define BATTERY_ADC_CHANNEL   2u
#define BATTERY_DIVIDER       2u      /* VBAT = V(A2) · 2 */
#define BATTERY_PRESENT_MV    3000u   /* sotto questa soglia il TS è considerato scollegato */

void Battery_Init(void);

/* Nuova misura + filtro. Da chiamare a periodo fisso (100 ms). */
void Battery_Update(void);

/* Tensione filtrata delle pile, in mV. */
uint16_t Battery_GetMv(void);

/* true se le pile AA sono collegate. */
bool Battery_IsPresent(void);

#endif /* BATTERY_H */
