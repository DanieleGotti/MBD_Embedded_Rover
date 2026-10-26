/*
 * comm — messaggi tra il rover e il PC / l'app.
 *
 *   ID    direzione     nome         payload
 *   0x01  PC → rover    HELLO        —
 *   0x02  rover → PC    HELLO_ACK    fw_major u8, fw_minor u8, proto u8, reset_cause u8
 *   0x10  PC → rover    DRIVE        drive_l i8, drive_r i8, speed u8      (≥ 10 Hz: è anche l'heartbeat)
 *   0x11  PC → rover    SERVO        deg i8, on u8
 *   0x20  PC → rover    SERVO_TRIM   delta i8
 *   0x21  PC → rover    SAVE_CALIB   —
 *   0x30  rover → PC    STATUS       vedi Comm_SendStatus (5 Hz)
 *
 * Se non arriva un DRIVE per COMM_LINK_TIMEOUT_MS, il link è considerato perso.
 */
#ifndef COMM_H
#define COMM_H

#include <stdbool.h>

#include "../rte/rte.h"

#define COMM_PROTO_VERSION    1u
#define COMM_LINK_TIMEOUT_MS  500u

#define MSG_HELLO       0x01u
#define MSG_HELLO_ACK   0x02u
#define MSG_DRIVE       0x10u
#define MSG_SERVO       0x11u
#define MSG_SERVO_TRIM  0x20u
#define MSG_SAVE_CALIB  0x21u
#define MSG_STATUS      0x30u

void Comm_Init(void);

/* Legge i byte ricevuti, decodifica i frame, aggiorna i comandi. Ogni 10 ms. */
void Comm_Process(void);

/* true se un DRIVE è arrivato negli ultimi COMM_LINK_TIMEOUT_MS */
bool Comm_IsLinkOk(void);

/* Invia il frame STATUS con lo stato dell'ultimo ciclo di controllo. */
void Comm_SendStatus(const Rte_Sensors *sen, const Rte_Actuators *act);

#endif /* COMM_H */
