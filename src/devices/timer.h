#ifndef DEVICES_TIMER_H
#define DEVICES_TIMER_H

#include <round.h>
#include <stdint.h>
#include "lib/kernel/list.h"

/* Number of timer interrupts per second. */
#define TIMER_FREQ 100

typedef struct queue_node {
    struct list_elem elem;
    struct thread * process;
    int64_t finish;
} queue_node;

void timer_init (void);
void timer_calibrate (void);

int64_t timer_ticks (void);
int64_t timer_elapsed (int64_t);

void timer_sleep (int64_t ticks);
void timer_msleep (int64_t milliseconds);
void timer_usleep (int64_t microseconds);
void timer_nsleep (int64_t nanoseconds);

void timer_print_stats (void);

bool thread_comp(const struct list_elem *l, const struct list_elem *r, void * aux);

#endif /* devices/timer.h */
