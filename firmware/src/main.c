/*
 * main — inizializzazione e task periodici gestiti dallo scheduler.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>

#include "mcal/tick.h"
#include "mcal/uart.h"
#include "services/scheduler.h"

/* Carico artificiale del task di prova, in ms: 0 = nessun carico.
 * Serve solo per vedere WCET e overrun cambiare. */
#define LOAD_MS 15

static void PrintU32(uint32_t value)
{
    char buf[11];

    ultoa(value, buf, 10);
    Uart_PutString(buf);
}

/* ---------------------------------------------------------------- task */

static void Task_Comm(void)
{
    char c;

    /* Eco: svuota tutto quello che è arrivato */
    while (Uart_GetChar(&c)) {
        Uart_PutChar(c);
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
