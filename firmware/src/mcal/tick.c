/*
 * tick — implementazione su Timer2 (CTC, prescaler 64, OCR2A = 249).
 */
#include "tick.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

/* Millisecondi da Tick_Init(); scritto dall'ISR, letto da main → volatile */
static volatile uint32_t s_ms = 0;

void Tick_Init(void)
{
    TCCR2A = (1 << WGM21);   /* modalità CTC */
    OCR2A  = 249;            /* 250 conteggi × 4 µs = 1 ms */
    TIMSK2 = (1 << OCIE2A);  /* interrupt su compare match A */
    TCCR2B = (1 << CS22);    /* prescaler 64: avvia il timer */
}

uint32_t Tick_GetMs(void)
{
    uint32_t ms;

    /* 4 byte letti uno alla volta: l'ISR non deve interrompere la copia */
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        ms = s_ms;
    }
    return ms;
}

uint32_t Tick_GetUs(void)
{
    uint32_t ms;
    uint8_t  cnt;
    uint8_t  pending;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        ms      = s_ms;
        cnt     = TCNT2;                    /* 0..249, 4 µs per conteggio */
        pending = TIFR2 & (1 << OCF2A);     /* compare avvenuto ma ISR non ancora eseguita */
    }

    /* Il timer è già ripartito da 0 ma s_ms non è stato ancora incrementato */
    if (pending && cnt < 125) {
        ms++;
    }
    return ms * 1000UL + (uint32_t)cnt * 4UL;
}

ISR(TIMER2_COMPA_vect)
{
    s_ms++;
}
