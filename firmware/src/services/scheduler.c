/*
 * scheduler — implementazione.
 */
#include "scheduler.h"

#include "../mcal/tick.h"

static Sched_Task *s_tasks = 0;
static uint8_t     s_count = 0;

void Sched_Init(Sched_Task *tasks, uint8_t count)
{
    uint32_t now = Tick_GetMs();

    s_tasks = tasks;
    s_count = count;

    for (uint8_t i = 0; i < count; i++) {
        tasks[i].next_ms  = now + tasks[i].offset_ms;
        tasks[i].last_us  = 0;
        tasks[i].wcet_us  = 0;
        tasks[i].overruns = 0;
    }
}

void Sched_RunOnce(void)
{
    for (uint8_t i = 0; i < s_count; i++) {
        Sched_Task *t = &s_tasks[i];

        if ((int32_t)(Tick_GetMs() - t->next_ms) < 0) {
            continue;
        }

        uint32_t start = Tick_GetUs();
        t->fn();
        uint32_t duration = Tick_GetUs() - start;

        if (duration > 65535) {
            duration = 65535;
        }
        t->last_us = (uint16_t)duration;
        
        if (duration > t->wcet_us) {
            t->wcet_us = (uint16_t)duration;
        }

        t->next_ms += t->period_ms;

        uint32_t now = Tick_GetMs();
        if ((int32_t)(now - t->next_ms) >= 0) {
            t->overruns++;
            t->next_ms = now + t->period_ms;
        }
    }
}
