/*
 * ultrasonic — implementazione.
 *
 * L'HC-SR04, dopo il trigger, alza ECHO per un tempo pari al viaggio
 * di andata e ritorno del suono. L'interrupt PCINT0 scatta su entrambi
 * i fronti di D8: sul fronte di salita salva l'istante, sul fronte di
 * discesa calcola la durata. I tempi vengono da Tick_GetUs() (4 µs).
 */
#include "ultrasonic.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <util/delay.h>

#include "../mcal/tick.h"

#define ECHO_MAX_US 25000UL    /* ≈ 4,3 m: oltre, consideriamo "nessun eco" */

static volatile uint32_t s_rise_us  = 0;   /* istante del fronte di salita */
static volatile uint32_t s_width_us = 0;   /* durata dell'eco */
static volatile uint8_t  s_done     = 0;   /* 1 = fronte di discesa arrivato */

void Ultrasonic_Init(void)
{
    PORTB &= ~(1 << PB4);      /* TRIG basso */
    DDRB  |=  (1 << PB4);      /* TRIG uscita */
    DDRB  &= ~(1 << PB0);      /* ECHO ingresso */

    PCMSK0 |= (1 << PCINT0);   /* pin PB0 */
    PCICR  |= (1 << PCIE0);    /* gruppo PCINT0..7 */
}

void Ultrasonic_Trigger(void)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        s_done = 0;
    }

    PORTB |= (1 << PB4);
    _delay_us(10);             /* unica attesa accettabile: 10 µs */
    PORTB &= ~(1 << PB4);
}

Us_Status Ultrasonic_GetResult(uint16_t *mm)
{
    uint32_t width;
    uint8_t  done;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        width = s_width_us;
        done  = s_done;
    }

    if (done == 0) {
        return US_BUSY;
    }

    if (width > ECHO_MAX_US) {
        return US_NO_ECHO;
    } else {
        *mm = (uint16_t)(width * 10UL / 58UL);
        return US_OK;
    }
}

ISR(PCINT0_vect)
{
    uint32_t now = Tick_GetUs();

    if (PINB & (1 << PB0)) {
        s_rise_us = now;
    } else {
        s_width_us = now - s_rise_us;
        s_done = 1;
    }
}
