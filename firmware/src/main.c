/*
 * main — inizializzazione e task periodici gestiti dallo scheduler.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>

#include "mcal/tick.h"
#include "mcal/uart.h"
#include "ecual/motor.h"
#include "ecual/servo.h"
#include "ecual/ultrasonic.h"
#include "ecual/battery.h"
#include "services/scheduler.h"

/* Carico artificiale del task di prova, in ms: 0 = nessun carico.
 * Serve solo per vedere WCET e overrun cambiare. */
#define LOAD_MS 0

static void PrintU32(uint32_t value)
{
    char buf[11];

    ultoa(value, buf, 10);
    Uart_PutString(buf);
}

/* Stampa un intero con segno */
static void PrintI32(int32_t value)
{
    if (value < 0) {
        Uart_PutChar('-');
        PrintU32((uint32_t)(-value));
    } else {
        PrintU32((uint32_t)value);
    }
}

/* Sposta il servo di delta gradi e stampa l'angolo effettivo (già saturato) */
static void ServoStep(int16_t delta)
{
    Servo_SetAngle(Servo_GetAngle() + delta);
    Uart_PutString("servo ");
    PrintI32(Servo_GetAngle());
    Uart_PutString("\r\n");
}

/* ---------------------------------------------------------------- task */

/* Velocità dei comandi da tastiera, 0 … MOTOR_MAX */
static int16_t s_speed = 120;

/* Direzione richiesta per ciascun motore: −1 indietro, 0 fermo, +1 avanti */
static int8_t s_dir_l = 0;
static int8_t s_dir_r = 0;

/* Applica ai motori lo stato corrente (direzioni × velocità) */
static void ApplyDrive(void)
{
    Motor_Set(s_dir_l * s_speed, s_dir_r * s_speed);
}

static void SetDirection(int8_t left, int8_t right)
{
    s_dir_l = left;
    s_dir_r = right;
    ApplyDrive();
}

/*
 * Comandi da tastiera (monitor seriale):
 *   w avanti   s indietro   a ruota a sinistra   d ruota a destra
 *   x (o spazio) stop        + / - velocità di 20 in 20
 *   j servo −10°   k servo al centro   l servo +10°   r rilascia servo
 */
static void Task_Comm(void)
{
    char c;

    while (Uart_GetChar(&c)) {
        switch (c) {
            case 'w':
                SetDirection(1, 1);
                Uart_PutString("avanti\r\n");
                break;
            case 'a':
                SetDirection(-1, 1);
                Uart_PutString("sinistra\r\n");
                break;
            case 's':
                SetDirection(-1, -1);
                Uart_PutString("indietro\r\n");
                break;
            case 'd':
                SetDirection(1, -1);
                Uart_PutString("destra\r\n");
                break;
            case '+':
                s_speed += 20;
                if (s_speed > MOTOR_MAX) {
                    s_speed = MOTOR_MAX;
                }
                ApplyDrive();
                PrintU32((uint32_t)s_speed);
                Uart_PutString(" (+)\r\n");
                break;
            case '-':
                s_speed -= 20;
                if (s_speed < 0) {
                    s_speed = 0;
                }
                ApplyDrive();
                PrintU32((uint32_t)s_speed);
                Uart_PutString(" (-)\r\n");
                break;
            case 'j':
                ServoStep(-10);
                break;
            case 'k':
                ServoStep(-Servo_GetAngle());   /* torna a 0 */
                break;
            case 'l':
                ServoStep(10);
                break;
            case 'r':
                Servo_Release();
                Uart_PutString("servo rilasciato\r\n");
                break;
            case 'x':
            case ' ':                  /* due tasti, stessa azione */
                SetDirection(0, 0);
                Uart_PutString("stop\r\n");
                break;
            default:                   /* qualunque altro tasto */
                break;
        }
    }
}

static void Task_Led(void)
{
    PINB = (1 << PB5);
}

static void Task_Load(void)
{
#if LOAD_MS > 0
    _delay_ms(LOAD_MS);
#endif
}

static void Task_Status(void);

/* ------------------------------------------------------ tabella dei task */

/* Ultima misura dell'ultrasuono */
static Us_Status s_us_status = US_BUSY;
static uint16_t  s_us_mm     = 0;

/* Ogni 60 ms: legge la misura precedente e ne avvia una nuova */
static void Task_Ultrasonic(void)
{
    s_us_status = Ultrasonic_GetResult(&s_us_mm);
    Ultrasonic_Trigger();
}

/* Ogni 100 ms: nuova misura della tensione delle pile */
static void Task_Battery(void)
{
    Battery_Update();
}

static Sched_Task s_tasks[] = {
    { .name = "comm",    .fn = Task_Comm,       .period_ms = 10,    .offset_ms = 0 },
    { .name = "led",     .fn = Task_Led,        .period_ms = 500,   .offset_ms = 1 },
    { .name = "load",    .fn = Task_Load,       .period_ms = 100,   .offset_ms = 2 },
    { .name = "status",  .fn = Task_Status,     .period_ms = 1000,  .offset_ms = 3 },
    { .name = "us",      .fn = Task_Ultrasonic, .period_ms = 60,    .offset_ms = 4 },
    { .name = "batt",    .fn = Task_Battery,    .period_ms = 100,   .offset_ms = 5 },
};

#define TASK_COUNT (sizeof(s_tasks) / sizeof(s_tasks[0]))

/* Stampa una riga per task: nome, durata ultima, WCET, overrun */
static void Task_Status(void)
{
    Uart_PutString("--- t = ");
    PrintU32(Tick_GetMs());
    Uart_PutString(" ms, tx persi = ");
    PrintU32(Uart_GetTxDropped());
    Uart_PutString(", distanza = ");
    if (s_us_status == US_OK) {
        PrintU32(s_us_mm);
        Uart_PutString(" mm");
    } else if (s_us_status == US_NO_ECHO) {
        Uart_PutString("nessun eco");
    } else {
        Uart_PutString("in corso");
    }
    Uart_PutString(", pile = ");
    PrintU32(Battery_GetMv());
    Uart_PutString(Battery_IsPresent() ? " mV" : " mV (scollegate)");
    Uart_PutString("\r\n");

    for (uint8_t i = 0; i < TASK_COUNT; i++) {
        Uart_PutString(s_tasks[i].name);
        Uart_PutString(": ultimo ");
        PrintU32(s_tasks[i].last_us);
        Uart_PutString(" us, max ");
        PrintU32(s_tasks[i].wcet_us);
        Uart_PutString(" us, overrun ");
        PrintU32(s_tasks[i].overruns);
        Uart_PutString("\r\n");
    }
}

/* ---------------------------------------------------------------- main */

int main(void)
{
    Motor_Init();            /* per primo: motori in stato sicuro */
    Servo_Init();            /* subito dopo: impulsi validi al servo */
    Ultrasonic_Init();
    Battery_Init();
    DDRB |= (1 << PB5);      /* LED di stato */

    Tick_Init();
    Uart_Init();
    Sched_Init(s_tasks, TASK_COUNT);

    sei();

    Uart_PutString("MBD_Embedded_Rover avviato\r\n");

    for (;;) {
        Sched_RunOnce();
    }
}
