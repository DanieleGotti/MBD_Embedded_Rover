/*
 * wdt — watchdog e causa dell'ultimo reset.
 *
 * Il watchdog è un timer indipendente: se il firmware non lo "rassicura"
 * (Wdt_Kick) entro il timeout, resetta il microcontrollore.
 */
#ifndef WDT_H
#define WDT_H

typedef enum {
    RESET_POWER_ON = 0,   /* accensione */
    RESET_EXTERNAL,       /* pulsante di reset, o apertura del monitor seriale */
    RESET_BROWN_OUT,      /* tensione di alimentazione scesa troppo */
    RESET_WATCHDOG,       /* firmware bloccato */
    RESET_UNKNOWN
} Wdt_ResetCause;

/* Avvia il watchdog con timeout di 60 ms. */
void Wdt_Enable(void);

/* "Sono vivo": riparte il conteggio del timeout. */
void Wdt_Kick(void);

/* Causa dell'ultimo reset, letta all'avvio prima di main(). */
Wdt_ResetCause Wdt_GetResetCause(void);

/* Nome leggibile della causa, per la stampa. */
const char *Wdt_ResetCauseName(Wdt_ResetCause cause);

#endif /* WDT_H */
