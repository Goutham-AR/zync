#include "internal.h"
#include <stdio.h>

int scheduler_init(Scheduler* s, size_t capacity) {
    if (!s || capacity == 0) {
        fprintf(stderr, "scheduler_init: invalid arguments\n");
        return -1;
    }

    s->coros = calloc(capacity, sizeof(Coroutine));
    if (!s->coros) {
        fprintf(stderr, "scheduler_init: alloc failed\n");
        return -1;
    }

    s->ready_q = malloc(capacity * sizeof(int));
    if (!s->ready_q) {
        free(s->coros);
        return -1;
    }

    s->free_slots = malloc(capacity * sizeof(int));
    if (!s->free_slots) {
        free(s->coros);
        free(s->ready_q);
        return -1;
    }

    s->capacity = capacity;
    s->high_water = 0;
    s->current_idx = -1;
    s->rq_head = s->rq_tail = s->rq_count = 0;
    s->free_count = 0;
    return 0;
}

void scheduler_destroy(Scheduler* s) {
    if (!s) return;
    for (size_t i = 0; i < s->high_water; ++i) {
        Coroutine* c = &s->coros[i];
        if (c->stack) {
            munmap(c->stack, STACK_SIZE + PAGE_SIZE);
            c->stack = NULL;
        }
    }
    free(s->coros);
    free(s->ready_q);
    free(s->free_slots);
    s->coros = NULL;
    s->ready_q = NULL;
    s->free_slots = NULL;
}

int run_loop(Scheduler* s) {
    if (!s) {
        fprintf(stderr, "run_loop: NULL scheduler\n");
        return -1;
    }
    while (s->rq_count > 0) {
        int idx = rq_dequeue(s);
        Coroutine* coro = &s->coros[idx];
        s->current_idx = idx;
        int res = swapcontext(&s->loop_ctx, &coro->context);
        s->current_idx = -1;
        if (res == -1) {
            fprintf(stderr, "swapcontext failed\n");
            return -1;
        }
        if (coro->done) {
            coro_free(coro);
            s->free_slots[s->free_count++] = idx;
        }
    }
    return 0;
}
