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

static Sched_Task s_tasks[] = {
    { .name = "comm",   .fn = Task_Comm,    .period_ms = 10,   .offset_ms = 0 },
    { .name = "led",    .fn = Task_Led,     .period_ms = 500,  .offset_ms = 1 },
    { .name = "load",   .fn = Task_Load,    .period_ms = 100,  .offset_ms = 2 },
    { .name = "status", .fn = Task_Status,  .period_ms = 1000, .offset_ms = 3 },
};

#define TASK_COUNT (sizeof(s_tasks) / sizeof(s_tasks[0]))

/* Stampa una riga per task: nome, durata ultima, WCET, overrun */
static void Task_Status(void)
{
    Uart_PutString("--- t = ");
    PrintU32(Tick_GetMs());
    Uart_PutString(" ms, tx persi = ");
    PrintU32(Uart_GetTxDropped());
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
