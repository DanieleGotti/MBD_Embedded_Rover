/*
 * crc8 — implementazione bit a bit.
 */
#include "crc8.h"

#define CRC8_POLY 0x07u

uint8_t Crc8_Update(uint8_t crc, uint8_t data)
{
    crc ^= data;
    for (uint8_t i = 0; i < 8; i++) {
        if (crc & 0x80) {
            crc = (uint8_t)((crc << 1) ^ CRC8_POLY);
        } else {
            crc = (uint8_t)(crc << 1);
        }
    }
    return crc;
}

uint8_t Crc8_Compute(const uint8_t *data, uint8_t len)
{
    uint8_t crc = CRC8_INIT;

    for (uint8_t i = 0; i < len; i++) {
        crc = Crc8_Update(crc, data[i]);
    }
    return crc;
}
