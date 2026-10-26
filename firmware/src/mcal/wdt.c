/*
 * wdt — implementazione.
 */
#include "wdt.h"

#include <avr/io.h>
#include <avr/wdt.h>

/*
 * Copia di MCUSR (registro con i flag della causa di reset), salvata
 * nella sezione .noinit: il codice di avvio non la azzera.
 */
static uint8_t s_mcusr __attribute__((section(".noinit")));

/*
 * Eseguita dal codice di avvio (sezione .init3), PRIMA di main().
 * Dopo un reset da watchdog, il watchdog resta acceso con il timeout
 * minimo (15 ms): se non lo spegniamo subito, il firmware si resetta
 * in continuazione prima ancora di arrivare a main().
 */
void Wdt_EarlyInit(void) __attribute__((naked, used, section(".init3")));
void Wdt_EarlyInit(void)
{
    uint8_t from_boot;

    /* Il bootloader Optiboot azzera MCUSR prima di avviare il firmware,
     * ma ne lascia una copia nel registro r2 della CPU */
    __asm__ volatile ("mov %0, r2" : "=r" (from_boot));

    s_mcusr = MCUSR | from_boot;
    MCUSR = 0;
    wdt_disable();
}

void Wdt_Enable(void)
{
    /* TODO 1: wdt_enable(...) con la costante del timeout da 60 ms
               (in <avr/wdt.h>: WDTO_15MS, WDTO_30MS, WDTO_60MS, …) */
    wdt_enable(WDTO_60MS);
}

void Wdt_Kick(void)
{
    /* TODO 2: wdt_reset() */
    wdt_reset();
}

Wdt_ResetCause Wdt_GetResetCause(void)
{
    /* TODO 3: controllare i bit di s_mcusr in quest'ordine e ritornare
               la prima causa trovata:
                 WDRF  → RESET_WATCHDOG
                 BORF  → RESET_BROWN_OUT
                 EXTRF → RESET_EXTERNAL
                 PORF  → RESET_POWER_ON
               nessuno → RESET_UNKNOWN */
    if (s_mcusr & (1 << WDRF)) {
        return RESET_WATCHDOG;
    } else if (s_mcusr & (1 << BORF)){
        return RESET_BROWN_OUT;
    } else if (s_mcusr & (1 << EXTRF)){
        return RESET_EXTERNAL;
    } else if (s_mcusr & (1 << PORF)){
        return RESET_POWER_ON;
    } else {
        return RESET_UNKNOWN;
    }
}

const char *Wdt_ResetCauseName(Wdt_ResetCause cause)
{
    switch (cause) {
    case RESET_POWER_ON:  return "accensione";
    case RESET_EXTERNAL:  return "reset esterno";
    case RESET_BROWN_OUT: return "brown-out";
    case RESET_WATCHDOG:  return "WATCHDOG";
    default:              return "sconosciuta";
    }
}
