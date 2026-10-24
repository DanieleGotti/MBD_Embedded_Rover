/*
 * battery — implementazione.
 *
 * Filtro passa-basso del primo ordine, a tempo discreto:
 *     y[k] = y[k−1] + (x[k] − y[k−1]) / 8
 * Con un campione ogni 100 ms la costante di tempo è ≈ 8 · 100 ms = 0,8 s.
 */
#include "battery.h"

#include <avr/io.h>

#include "../mcal/adc.h"

static int32_t s_filtered_mv = 0;
static bool    s_first       = true;

/* Converte un valore ADC grezzo (0…1023) nella tensione delle pile in mV */
static uint16_t RawToMv(uint16_t raw)
{
    uint32_t mv = (uint32_t)raw * ADC_VREF_MV / ADC_MAX;
    return (uint16_t)(mv * BATTERY_DIVIDER);
}

void Battery_Init(void)
{
    Adc_Init();
    DIDR0 |= (1 << ADC2D);   /* A2 solo analogico: buffer digitale spento */
}

void Battery_Update(void)
{
    uint16_t mv = RawToMv(Adc_Read(BATTERY_ADC_CHANNEL));

    if (s_first) {
        s_filtered_mv = mv;
        s_first = false;
    } else {
        s_filtered_mv += ((int32_t)mv - s_filtered_mv) / 8;
    }
}

uint16_t Battery_GetMv(void)
{
    return (uint16_t)s_filtered_mv;
}

bool Battery_IsPresent(void)
{
    return Battery_GetMv() > BATTERY_PRESENT_MV;
}
