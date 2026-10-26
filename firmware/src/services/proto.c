/*
 * proto — implementazione.
 *
 * Il parser è una macchina a stati: ogni byte fa avanzare lo stato.
 * Se qualcosa non torna, si torna ad aspettare 0xAA: così ci si
 * risincronizza da soli dopo un byte perso o corrotto.
 */
#include "proto.h"

#include "crc8.h"
#include "../mcal/uart.h"

typedef enum {
    ST_WAIT_SOF1,   /* aspetto 0xAA */
    ST_WAIT_SOF2,   /* aspetto 0x55 */
    ST_ID,
    ST_LEN,
    ST_PAYLOAD,
    ST_CRC
} Parser_State;

static Parser_State s_state  = ST_WAIT_SOF1;
static Proto_Frame  s_frame;            /* frame in costruzione */
static uint8_t      s_index  = 0;       /* byte di payload ricevuti */
static uint8_t      s_crc    = CRC8_INIT;
static uint16_t     s_errors = 0;

void Proto_Init(void)
{
    s_state = ST_WAIT_SOF1;
}

bool Proto_ParseByte(uint8_t byte, Proto_Frame *out)
{
    switch (s_state) {
    case ST_WAIT_SOF1:
        if (byte == PROTO_SOF1) {
            s_state = ST_WAIT_SOF2;
        }
        break;

    case ST_WAIT_SOF2:
        if (byte == PROTO_SOF2) {
            s_state = ST_ID;
            s_crc = CRC8_INIT;
        } else if (byte == PROTO_SOF1) {
            s_state = ST_WAIT_SOF2;
        } else {
            s_state = ST_WAIT_SOF1;
        }
        break;

    case ST_ID:
        s_frame.id = byte;
        s_crc = Crc8_Update(s_crc, byte);
        s_state = ST_LEN;
        break;

    case ST_LEN:
        if (byte > PROTO_MAX_PAYLOAD) {
            s_errors++;
            s_state = ST_WAIT_SOF1;
        } else {
            s_frame.len = byte;
            s_crc = Crc8_Update(s_crc, byte);
            s_index = 0;
            s_state = ST_PAYLOAD;
            if (s_frame.len == 0) {
                s_state = ST_CRC;
            }
        }
        break;

    case ST_PAYLOAD:
        s_frame.payload[s_index] = byte;
        s_index++;
        s_crc = Crc8_Update(s_crc, byte);
        if (s_index == s_frame.len) {
            s_state = ST_CRC;
        }
        break;

    case ST_CRC:
        s_state = ST_WAIT_SOF1;
        if (byte == s_crc) {
            *out = s_frame;
            return true;
        } else {
            s_errors++;
        }
        break;

    default:
        s_state = ST_WAIT_SOF1;
        break;
    }
    return false;
}

void Proto_Send(uint8_t id, const uint8_t *payload, uint8_t len)
{
    uint8_t crc = CRC8_INIT;

    crc = Crc8_Update(crc, id);
    crc = Crc8_Update(crc, len);
    for (uint8_t i = 0; i < len; i++) {
        crc = Crc8_Update(crc, payload[i]);
    }

    Uart_PutChar((char)PROTO_SOF1);
    Uart_PutChar((char)PROTO_SOF2);
    Uart_PutChar((char)id);
    Uart_PutChar((char)len);
    for (uint8_t i = 0; i < len; i++) {
        Uart_PutChar((char)payload[i]);
    }
    Uart_PutChar((char)crc);
}

uint16_t Proto_GetErrors(void)
{
    return s_errors;
}
