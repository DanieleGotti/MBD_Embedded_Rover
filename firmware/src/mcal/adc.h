/*
 * adc — convertitore analogico-digitale a 10 bit.
 *
 * Riferimento AVcc (5 V), clock ADC = 16 MHz / 128 = 125 kHz,
 * una conversione dura 13 cicli ≈ 104 µs.
 */
#ifndef ADC_H
#define ADC_H

#include <stdint.h>

#define ADC_MAX     1023u   /* valore a fondo scala */
#define ADC_VREF_MV 5000u   /* tensione di riferimento nominale (AVcc) */

/* Accende l'ADC e lo configura. */
void Adc_Init(void);

/*
 * Converte il canale indicato (0…7 = A0…A7) e ritorna il valore 0…1023.
 * Attende la fine della conversione: circa 104 µs.
 */
uint16_t Adc_Read(uint8_t channel);

#endif /* ADC_H */
