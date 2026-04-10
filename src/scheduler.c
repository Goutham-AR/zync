#include "internal.h"
#include <stdio.h>

void scheduler_init(Scheduler* s) {
    if (!s) return;
    s->current_idx = -1;
    s->total_spawned = 0;
}

void scheduler_destroy(Scheduler* s) {
    if (!s) return;
    for (size_t i = 0; i < s->total_spawned; ++i) {
        Coroutine *c = &s->coros[i];
        if (c->stack) {
            munmap(c->stack, STACK_SIZE + PAGE_SIZE);
            c->stack = NULL;
        }
    }
}

int run_loop(Scheduler* s) {
    if (!s) {
        fprintf(stderr, "run_loop: NULL scheduler\n");
        return -1;
    }
    int to_complete;
    do {
        to_complete = 0;
        size_t count = s->total_spawned;
        for (int i = 0; i < (int)count; ++i) {
            Coroutine* coro = &s->coros[i];
            if (!coro->done) {
                ++to_complete;
                s->current_idx = i;
                int res = swapcontext(&s->loop_ctx, &coro->context);
                if (res == -1) {
                    fprintf(stderr, "swapcontext failed\n");
                    return -1;
                }
            } else {
                coro_free(coro);
            }
        }
    } while (to_complete != 0);
    return 0;
}
