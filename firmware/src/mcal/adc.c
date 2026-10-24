/*
 * adc — implementazione a polling.
 *
 * 104 µs di attesa sono accettabili dentro un task da 100 ms:
 * un interrupt qui complicherebbe il codice senza vantaggi.
 */
#include "adc.h"

#include <avr/io.h>

void Adc_Init(void)
{
    ADMUX = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

uint16_t Adc_Read(uint8_t channel)
{
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

    ADCSRA |= (1 << ADSC);

    while (ADCSRA & (1 << ADSC)) {
        /* circa 104 µs */
    }

    return ADC;
}
