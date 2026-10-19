/*
 * tick — base dei tempi di sistema a 1 ms (Timer2, modalità CTC).
 *
 * Risorse usate: Timer2, interrupt TIMER2_COMPA.
 */
#ifndef TICK_H
#define TICK_H

#include <stdint.h>

/*
 * Configura Timer2 per generare un interrupt ogni 1 ms.
 * Non abilita gli interrupt globali: lo fa main() con sei()
 * dopo aver inizializzato tutti i moduli.
 */
void Tick_Init(void);

/*
 * Millisecondi trascorsi da Tick_Init().
 * Sicura da chiamare dal codice principale anche con gli interrupt attivi.
 * Si azzera dopo circa 49,7 giorni (overflow di uint32_t).
 */
uint32_t Tick_GetMs(void);

#endif /* TICK_H */
