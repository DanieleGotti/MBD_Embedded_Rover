/*
 * comm — implementazione.
 */
#include "comm.h"

#include "proto.h"
#include "calib.h"
#include "../mcal/tick.h"
#include "../mcal/uart.h"
#include "../mcal/wdt.h"
#include "../ecual/servo.h"

#define FW_MAJOR 0u
#define FW_MINOR 11u

/* STATUS: flag nel byte 'flags' */
#define FLAG_TS_PRESENT  (1u << 0)
#define FLAG_DIST_VALID  (1u << 1)
#define FLAG_AEB_ACTIVE  (1u << 2)
#define FLAG_LINK_OK     (1u << 3)
#define FLAG_SERVO_ON    (1u << 4)

static uint32_t s_last_drive_ms = 0;
static bool     s_ever_driven   = false;

/* Frame DRIVE ricevuti: il PC li confronta con quelli inviati per stimare le perdite */
static uint16_t s_frame_counter = 0;

/* Scrittura little-endian in un buffer */
static void PutU16(uint8_t *buf, uint8_t *pos, uint16_t v)
{
    buf[(*pos)++] = (uint8_t)(v & 0xFFu);
    buf[(*pos)++] = (uint8_t)(v >> 8);
}

static void PutU32(uint8_t *buf, uint8_t *pos, uint32_t v)
{
    PutU16(buf, pos, (uint16_t)(v & 0xFFFFu));
    PutU16(buf, pos, (uint16_t)(v >> 16));
}

static void HandleFrame(const Proto_Frame *f)
{
    Rte_Commands *cmd = Rte_GetCommands();

    switch (f->id) {
    case MSG_HELLO: {
        uint8_t ack[4] = {
            FW_MAJOR, FW_MINOR, COMM_PROTO_VERSION, (uint8_t)Wdt_GetResetCause()
        };
        Proto_Send(MSG_HELLO_ACK, ack, sizeof(ack));
        break;
    }
    case MSG_DRIVE:
        if (f->len == 3) {
            cmd->drive_l = (int8_t)f->payload[0];
            cmd->drive_r = (int8_t)f->payload[1];
            cmd->speed   = f->payload[2];
            s_last_drive_ms = Tick_GetMs();
            s_ever_driven   = true;
            s_frame_counter++;
        }
        break;
    case MSG_SERVO:
        if (f->len == 2) {
            cmd->servo_deg = (int8_t)f->payload[0];
            cmd->servo_on  = (f->payload[1] != 0);
        }
        break;
    case MSG_SERVO_TRIM:
        if (f->len == 1) {
            Calib_Get()->servo_trim_deg += (int8_t)f->payload[0];
            Servo_SetTrim(Calib_Get()->servo_trim_deg);
        }
        break;
    case MSG_SAVE_CALIB:
        Calib_Get()->speed_default = cmd->speed;
        Calib_Save();
        break;
    default:
        break;   /* messaggio sconosciuto: ignorato */
    }
}

void Comm_Init(void)
{
    Proto_Init();
}

void Comm_Process(void)
{
    char        c;
    Proto_Frame frame;

    while (Uart_GetChar(&c)) {
        if (Proto_ParseByte((uint8_t)c, &frame)) {
            HandleFrame(&frame);
        }
    }
}

bool Comm_IsLinkOk(void)
{
    return s_ever_driven &&
           (uint32_t)(Tick_GetMs() - s_last_drive_ms) < COMM_LINK_TIMEOUT_MS;
}

/*
 * STATUS (21 byte):
 *   time_ms u32, vbat_mv u16, dist_mm u16, flags u8,
 *   pwm_l i16, pwm_r i16, servo_deg i8, servo_trim i8,
 *   tx_dropped u16, proto_errors u16, drive_rx u16
 */
void Comm_SendStatus(const Rte_Sensors *sen, const Rte_Actuators *act)
{
    uint8_t buf[21];
    uint8_t pos   = 0;
    uint8_t flags = 0;

    if (sen->ts_present)  flags |= FLAG_TS_PRESENT;
    if (sen->dist_valid)  flags |= FLAG_DIST_VALID;
    if (act->aeb_active)  flags |= FLAG_AEB_ACTIVE;
    if (Comm_IsLinkOk())  flags |= FLAG_LINK_OK;
    if (act->servo_on)    flags |= FLAG_SERVO_ON;

    PutU32(buf, &pos, sen->time_ms);
    PutU16(buf, &pos, sen->vbat_mv);
    PutU16(buf, &pos, sen->dist_mm);
    buf[pos++] = flags;
    PutU16(buf, &pos, (uint16_t)act->pwm_l);
    PutU16(buf, &pos, (uint16_t)act->pwm_r);
    buf[pos++] = (uint8_t)(int8_t)act->servo_deg;
    buf[pos++] = (uint8_t)Calib_Get()->servo_trim_deg;
    PutU16(buf, &pos, Uart_GetTxDropped());
    PutU16(buf, &pos, Proto_GetErrors());
    PutU16(buf, &pos, s_frame_counter);

    Proto_Send(MSG_STATUS, buf, pos);
}
