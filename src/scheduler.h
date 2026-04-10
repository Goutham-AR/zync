#pragma once

#include <stddef.h>

#include "coro.h"

#define MAX_COROS 20


typedef struct {
    Coroutine coros[MAX_COROS];
    ucontext_t loop_ctx;
    size_t total_spawned;
    int current_idx;
    Callback pending_fn;
} Scheduler;


void scheduler_init(Scheduler*);
void scheduler_destroy(Scheduler*);
