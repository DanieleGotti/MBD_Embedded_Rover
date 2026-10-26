/*
 * rte — Runtime Environment: il confine tra il software di base (BSW)
 * e la logica applicativa (ASW).
 *
 * L'applicazione non vede né registri né driver: riceve tre strutture
 * di segnali (comandi, sensori) e ne produce una (attuatori).
 * È la stessa interfaccia che avrà il codice generato da Embedded Coder:
 *     ingressi → step() → uscite
 *
 * Unità: mm, mV, gradi, ms. PWM nel range −255 … +255.
 */
#ifndef RTE_H
#define RTE_H

#include <stdbool.h>
#include <stdint.h>

/* Comandi dell'operatore: oggi dalla tastiera, domani dall'app */
typedef struct {
    int8_t  drive_l;       /* −1 indietro, 0 fermo, +1 avanti */
    int8_t  drive_r;
    uint8_t speed;         /* 0 … 255 */
    int16_t servo_deg;     /* angolo richiesto */
    bool    servo_on;      /* false = servo rilasciato */
    bool    link_ok;       /* comandi ricevuti di recente (heartbeat) */
} Rte_Commands;

/* Misure dei sensori, in unità fisiche */
typedef struct {
    uint32_t time_ms;
    uint16_t vbat_mv;
    bool     ts_present;   /* pile AA collegate */
    uint16_t dist_mm;
    bool     dist_valid;   /* false = nessun eco o misura in corso */
} Rte_Sensors;

/* Comandi verso gli attuatori */
typedef struct {
    int16_t pwm_l;         /* −255 … +255 */
    int16_t pwm_r;
    int16_t servo_deg;
    bool    servo_on;
    bool    aeb_active;    /* frenata automatica intervenuta */
} Rte_Actuators;

/* Comandi correnti: li scrive il modulo di comunicazione, li legge il controllo */
Rte_Commands *Rte_GetCommands(void);

/* Ciclo dell'ultrasuono: legge la misura precedente e ne avvia una nuova (ogni 60 ms) */
void Rte_UltrasonicCycle(void);

/* Riempie la struttura dei sensori con le ultime misure */
void Rte_ReadSensors(Rte_Sensors *sen);

/* Applica le uscite ai driver */
void Rte_WriteActuators(const Rte_Actuators *act);

#endif /* RTE_H */
