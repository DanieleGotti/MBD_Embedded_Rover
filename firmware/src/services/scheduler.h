/*
 * scheduler — scheduler cooperativo a task periodici.
 *
 * Ogni task è una funzione void f(void) eseguita ogni period_ms.
 * "Cooperativo": un task non viene mai interrotto da un altro task,
 * quindi deve finire in fretta (niente attese al suo interno).
 * Per ogni task vengono misurati durata dell'ultima esecuzione,
 * durata massima (WCET) e numero di overrun.
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

/* Puntatore a funzione: "indirizzo di una funzione void f(void)" */
typedef void (*Sched_TaskFn)(void);

typedef struct {
    /* Configurazione: la scrive l'applicazione */
    const char  *name;
    Sched_TaskFn fn;
    uint16_t     period_ms;
    uint16_t     offset_ms;   /* ritardo della prima esecuzione, per non far partire tutti insieme */

    /* Stato interno e statistiche: li scrive lo scheduler */
    uint32_t     next_ms;     /* istante della prossima esecuzione */
    uint16_t     last_us;     /* durata dell'ultima esecuzione */
    uint16_t     wcet_us;     /* durata massima osservata */
    uint16_t     overruns;    /* volte in cui il task è finito oltre la scadenza successiva */
} Sched_Task;

/* Registra la tabella dei task e azzera le statistiche. Da chiamare dopo Tick_Init(). */
void Sched_Init(Sched_Task *tasks, uint8_t count);

/* Esegue una volta i task scaduti. Da chiamare in continuazione nel for (;;). */
void Sched_RunOnce(void);

#endif /* SCHEDULER_H */
