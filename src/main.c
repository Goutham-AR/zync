#define _XOPEN_SOURCE
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ucontext.h>

#include "coro.h"
#include "scheduler.h"

#define STACK_SIZE (64 * 1024)

Scheduler scheduler;

int yield(Scheduler* s) {
    Coroutine* coros = s->coros;
    int current_idx = s->current_idx;
    ucontext_t* loop_ctx = &s->loop_ctx;
    int res = swapcontext(&coros[current_idx].context, loop_ctx);
    if (res == -1) {
        fprintf(stderr, "swapcontext failed\n");
        return -1;
    }
    return 0;
}

void counter(int id) {
    for (int i = 0; i < 3; ++i) {
        printf("[%d] %d\n", id, i);
        assert(!yield(&scheduler));
    }
}

void do_coro(int high, int low, int arg) {
    uintptr_t ptr = (uintptr_t)((uintptr_t)high << 32 | low);
    Scheduler* s = (Scheduler*)ptr;
    Callback pending_fn = s->pending_fn;
    (*pending_fn)(arg);
    s->coros[s->current_idx].done = true;
}

void run_loop(Scheduler* s) {
    int to_complete;
    Coroutine* coros = s->coros;
    ucontext_t* loop_ctx = &s->loop_ctx;
    do {
        to_complete = 0;
        size_t count = s->total_spawned;
        for (int i = 0; i < count; ++i) {
            if (!coros[i].done) {
                ++to_complete;
                s->pending_fn = coros[i].cb;
                s->current_idx = i;
                int res = swapcontext(loop_ctx, &coros[i].context);
                if (res == -1) {
                    fprintf(stderr, "swapcontext failed\n");
                }
            } else {
                coro_free(&coros[i]);
            }
        }
        // printf("to_complete = %d\n", to_complete);
    } while (to_complete != 0);
}

int spawn(Scheduler* s, Callback cb, int inp) {
    Coroutine* coros = s->coros;
    ucontext_t* loop_ctx = &s->loop_ctx;
    assert(s->total_spawned < MAX_COROS);
    if (s->total_spawned >= MAX_COROS) {
        fprintf(stderr, "spawn: max coroutines reached\n");
        return -1;
    }
    int res = getcontext(&coros[s->total_spawned].context); // check the error
    if (res == -1) {
        fprintf(stderr, "getcontext failed\n");
        return -1;
    }
    coros[s->total_spawned].done = false;
    uint8_t* stack = malloc(STACK_SIZE); // check the error
    if (!stack) {
        fprintf(stderr, "malloc failed\n");
        return -1;
    }
    coros[s->total_spawned].context.uc_stack.ss_size = STACK_SIZE;
    coros[s->total_spawned].context.uc_stack.ss_sp = stack;
    coros[s->total_spawned].stack = stack;
    coros[s->total_spawned].context.uc_link = loop_ctx;
    coros[s->total_spawned].cb = cb;

    uintptr_t ptr = (uintptr_t)s;
    int low = (int)(ptr & 0xFFFFFFFF);
    int high = (int)(ptr >> 32);
    makecontext(&coros[s->total_spawned].context, do_coro, 3, high, low, inp);
    ++s->total_spawned;
    return 0;
}

int main() {
    scheduler_init(&scheduler);
    assert(!spawn(&scheduler, counter, 1));
    assert(!spawn(&scheduler, counter, 2));
    assert(!spawn(&scheduler, counter, 3));

    run_loop(&scheduler);

    scheduler_destroy(&scheduler);
}

