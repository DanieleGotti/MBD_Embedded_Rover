/*
 * proto — frame binari su UART.
 *
 *   ┌──────┬──────┬──────┬──────┬───────────────┬──────┐
 *   │ 0xAA │ 0x55 │  ID  │ LEN  │ PAYLOAD (LEN) │ CRC8 │
 *   └──────┴──────┴──────┴──────┴───────────────┴──────┘
 *   CRC8 calcolato su ID + LEN + PAYLOAD. Numeri multi-byte little-endian.
 */
#ifndef PROTO_H
#define PROTO_H

#include <stdbool.h>
#include <stdint.h>

#define PROTO_SOF1        0xAAu
#define PROTO_SOF2        0x55u
#define PROTO_MAX_PAYLOAD 24u

typedef struct {
    uint8_t id;
    uint8_t len;
    uint8_t payload[PROTO_MAX_PAYLOAD];
} Proto_Frame;

/* Riporta il parser allo stato iniziale. */
void Proto_Init(void);

/*
 * Dà un byte ricevuto al parser. Ritorna true quando un frame completo
 * e con CRC corretto è stato scritto in *out.
 * I frame corrotti vengono scartati e contati.
 */
bool Proto_ParseByte(uint8_t byte, Proto_Frame *out);

/* Costruisce e mette in coda di trasmissione un frame. */
void Proto_Send(uint8_t id, const uint8_t *payload, uint8_t len);

/* Frame scartati per CRC sbagliato o lunghezza eccessiva. */
uint16_t Proto_GetErrors(void);

#endif /* PROTO_H */
