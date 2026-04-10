#pragma once

#include <ucontext.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_COROS 20

typedef struct Scheduler Scheduler;

typedef void (*CoroFn)(Scheduler *s, void *userdata);

typedef struct {
    ucontext_t context;
    bool done;
    uint8_t *stack;
    CoroFn cb;
    void *arg;
    int self_idx;
} Coroutine;

struct Scheduler {
    Coroutine coros[MAX_COROS];
    ucontext_t loop_ctx;
    size_t total_spawned;
    int current_idx;
};

void scheduler_init(Scheduler *s);
void scheduler_destroy(Scheduler *s);

int spawn(Scheduler *s, CoroFn cb, void *arg);
int yield(Scheduler *s);
int run_loop(Scheduler *s);
