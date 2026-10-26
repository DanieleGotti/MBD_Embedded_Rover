/*
 * timer1 — implementazione.
 *
 * I registri a 16 bit (OCR1A, ICR1, TCNT1) si leggono e scrivono in due
 * passi tramite un registro temporaneo condiviso: se un interrupt accede a
 * un altro registro a 16 bit di Timer1 a metà operazione, il valore si
 * corrompe. Per questo le scritture fuori da Timer1_Init sono atomiche.
 */
#include "timer1.h"

#include <avr/io.h>
#include <util/atomic.h>

void Timer1_Init(void)
{
    ICR1 = TIMER1_TOP;
    OCR1A = 1500u * TIMER1_TICKS_PER_US;
    DDRB |= (1 << PB1);
    TCCR1A = (1 << COM1A1) | (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
}

void Timer1_SetCompareA(uint16_t ticks)
{
    if (ticks > TIMER1_TOP) {
        ticks = TIMER1_TOP;
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        OCR1A = ticks;
    }
}

void Timer1_SetOutputA(bool enable)
{
    if (enable) {
        TCCR1A |= (1 << COM1A1);    /* D9 comandato dal timer */
    } else {
        TCCR1A &= ~(1 << COM1A1);   /* D9 torna un GPIO normale ... */
        PORTB  &= ~(1 << PB1);      /* ... fisso a 0 */
    }
}

uint16_t Timer1_GetCount(void)
{
    uint16_t count;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        count = TCNT1;
    }
    return count;
}
