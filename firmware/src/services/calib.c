/*
 * calib — implementazione.
 */
#include "calib.h"

#include <avr/eeprom.h>

/* La struttura in EEPROM, all'indirizzo 0. EEMEM = "questa variabile sta in EEPROM". */
static Calib_Data EEMEM s_ee;

/* Copia di lavoro in RAM */
static Calib_Data s_cal;

static const Calib_Data k_defaults = {
    .magic          = CALIB_MAGIC,
    .version        = CALIB_VERSION,
    .speed_default  = 200,
    .servo_trim_deg = 0,
    .checksum       = 0,
};

/*
 * Checksum: somma di tutti i byte della struttura TRANNE l'ultimo
 * (il checksum stesso), troncata a 8 bit, poi invertita (~).
 */
static uint8_t Checksum(const Calib_Data *d)
{
    const uint8_t *bytes = (const uint8_t *)d;
    uint8_t sum = 0;

    for (uint8_t i = 0; i < sizeof(Calib_Data) - 1; i++) {   /* tutti tranne il checksum */
        sum += bytes[i];
    }

    return (uint8_t)~sum;
}

bool Calib_Load(void)
{
    eeprom_read_block(&s_cal, &s_ee, sizeof(s_cal));

    if (s_cal.magic != CALIB_MAGIC ||
        s_cal.version != CALIB_VERSION ||
        s_cal.checksum != Checksum(&s_cal)) {
        s_cal = k_defaults;
        return false;
    }
    return true;
}

void Calib_Save(void)
{
    s_cal.checksum = Checksum(&s_cal);
    eeprom_update_block(&s_cal, &s_ee, sizeof(s_cal));
}

Calib_Data *Calib_Get(void)
{
    return &s_cal;
}
