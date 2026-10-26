/*
 * main — inizializzazione e task periodici.
 *
 * Flusso dei dati:
 *   UART → Comm_Process → comandi RTE ───┐
 *   driver → Rte_ReadSensors ─────────────┼→ App_Step → Rte_WriteActuators → driver
 *                                          (Task_Control, ogni 20 ms)
 *   stato → Comm_SendStatus → UART (ogni 200 ms)
 *
 * Sulla UART passano solo frame binari: per comandare il rover dal PC
 * si usa tools/rover_console.py, non il monitor seriale.
 */
#include <avr/io.h>
#include <avr/interrupt.h>

#include "mcal/tick.h"
#include "mcal/uart.h"
#include "mcal/wdt.h"
#include "ecual/motor.h"
#include "ecual/servo.h"
#include "ecual/ultrasonic.h"
#include "ecual/battery.h"
#include "services/scheduler.h"
#include "services/calib.h"
#include "services/comm.h"
#include "rte/rte.h"
#include "app/app.h"

/* Ultimo ciclo di controllo, inviato nel frame STATUS */
static Rte_Sensors   s_sen;
static Rte_Actuators s_act;

static void Task_Comm(void)
{
    Comm_Process();
}

static void Task_Control(void)
{
    Rte_Commands *cmd = Rte_GetCommands();

    cmd->link_ok = Comm_IsLinkOk();
    Rte_ReadSensors(&s_sen);
    App_Step(cmd, &s_sen, &s_act);
    Rte_WriteActuators(&s_act);
}

static void Task_Ultrasonic(void)
{
    Rte_UltrasonicCycle();
}

static void Task_Battery(void)
{
    Battery_Update();
}

static void Task_Telemetry(void)
{
    Comm_SendStatus(&s_sen, &s_act);
}

static void Task_Led(void)
{
    /* Lampeggio lento: in attesa di comandi. Acceso fisso: link attivo. */
    if (Comm_IsLinkOk()) {
        PORTB |= (1 << PB5);
    } else {
        PINB = (1 << PB5);
    }
}

static Sched_Task s_tasks[] = {
    { .name = "comm",    .fn = Task_Comm,       .period_ms = 10,            .offset_ms = 0 },
    { .name = "ctrl",    .fn = Task_Control,    .period_ms = APP_PERIOD_MS, .offset_ms = 1 },
    { .name = "us",      .fn = Task_Ultrasonic, .period_ms = 60,            .offset_ms = 2 },
    { .name = "batt",    .fn = Task_Battery,    .period_ms = 100,           .offset_ms = 3 },
    { .name = "tlm",     .fn = Task_Telemetry,  .period_ms = 200,           .offset_ms = 4 },
    { .name = "led",     .fn = Task_Led,        .period_ms = 500,           .offset_ms = 5 },
};

#define TASK_COUNT (sizeof(s_tasks) / sizeof(s_tasks[0]))

int main(void)
{
    Motor_Init();            /* per primo: motori in stato sicuro */
    Servo_Init();            /* subito dopo: impulsi validi al servo */
    Ultrasonic_Init();
    Battery_Init();
    DDRB |= (1 << PB5);      /* LED di stato */

    Tick_Init();
    Uart_Init();
    Comm_Init();

    Calib_Load();
    Rte_GetCommands()->speed = Calib_Get()->speed_default;
    Servo_SetTrim(Calib_Get()->servo_trim_deg);

    App_Init();
    Sched_Init(s_tasks, TASK_COUNT);

    sei();
    Wdt_Enable();

    for (;;) {
        Sched_RunOnce();
        Wdt_Kick();
    }
}
