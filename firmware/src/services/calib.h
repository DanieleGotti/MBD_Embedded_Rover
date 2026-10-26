/*
 * calib — parametri di calibrazione salvati in EEPROM.
 *
 * All'avvio Calib_Load() legge la EEPROM; se i dati non sono validi
 * (EEPROM vuota, versione diversa, checksum sbagliato) usa i default.
 * Le modifiche restano in RAM finché non si chiama Calib_Save().
 */
#ifndef CALIB_H
#define CALIB_H

#include <stdbool.h>
#include <stdint.h>

#define CALIB_MAGIC   0xCA1Bu   /* "firma" che riconosce i nostri dati */
#define CALIB_VERSION 1u        /* da incrementare quando cambia la struttura */

typedef struct {
    uint16_t magic;
    uint8_t  version;
    uint8_t  speed_default;     /* velocità iniziale dei comandi da tastiera */
    int8_t   servo_trim_deg;    /* correzione del centro del servo */
    uint8_t  checksum;          /* sempre l'ultimo campo */
} Calib_Data;

/* Carica dalla EEPROM. Ritorna false se ha dovuto usare i default. */
bool Calib_Load(void);

/* Scrive in EEPROM i valori correnti (con checksum aggiornato). */
void Calib_Save(void);

/* Accesso ai valori correnti, in RAM (lettura e modifica). */
Calib_Data *Calib_Get(void);

#endif /* CALIB_H */
