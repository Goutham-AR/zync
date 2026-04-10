#pragma once

#include <ucontext.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define CORO_DEFAULT_CAPACITY 20

typedef struct Scheduler Scheduler;

typedef void (*CoroFn)(Scheduler *s, void *userdata);

typedef struct {
    ucontext_t  context;
    bool        done;
    uint8_t    *stack;
    CoroFn      cb;
    void       *arg;
    int         self_idx;
} Coroutine;

struct Scheduler {
    Coroutine  *coros;          /* dynamically allocated slot array        */
    size_t      capacity;       /* total number of slots                   */
    size_t      high_water;     /* next fresh slot index (never decreases) */
    int         current_idx;    /* slot index of the running coroutine     */
    ucontext_t  loop_ctx;

    /* ready-queue: circular buffer of runnable slot indices */
    int        *ready_q;
    size_t      rq_head;
    size_t      rq_tail;
    size_t      rq_count;

    /* free-list: recycled slot indices available for reuse */
    int        *free_slots;
    size_t      free_count;
};

int  scheduler_init(Scheduler *s, size_t capacity);
void scheduler_destroy(Scheduler *s);

int spawn(Scheduler *s, CoroFn cb, void *arg);
int yield(Scheduler *s);
int run_loop(Scheduler *s);
