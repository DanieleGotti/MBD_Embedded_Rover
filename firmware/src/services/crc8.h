/*
 * crc8 — CRC a 8 bit, polinomio 0x07 (x^8 + x^2 + x + 1), valore iniziale 0x00.
 *
 * Verifica: il CRC dei 9 byte ASCII "123456789" deve valere 0xF4.
 */
#ifndef CRC8_H
#define CRC8_H

#include <stdint.h>

#define CRC8_INIT 0x00u

/* Aggiorna il CRC con un byte: crc = Crc8_Update(crc, byte) */
uint8_t Crc8_Update(uint8_t crc, uint8_t data);

/* CRC di un intero buffer, partendo da CRC8_INIT */
uint8_t Crc8_Compute(const uint8_t *data, uint8_t len);

#endif /* CRC8_H */
