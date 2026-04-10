#include "internal.h"
#include <stdio.h>
#include <stdint.h>

void coro_free(Coroutine *c) {
    if (c->stack) {
        munmap(c->stack, STACK_SIZE + PAGE_SIZE);
        c->stack = NULL;
    }
    c->done = false;
}

int yield(Scheduler *s) {
    if (!s) {
        fprintf(stderr, "yield: NULL scheduler\n");
        return -1;
    }
    if (s->current_idx == -1) {
        fprintf(stderr, "yield: called outside a coroutine\n");
        return -1;
    }
    rq_enqueue(s, s->current_idx);
    int res = swapcontext(&s->coros[s->current_idx].context, &s->loop_ctx);
    if (res == -1) {
        fprintf(stderr, "swapcontext failed\n");
        return -1;
    }
    return 0;
}

static void do_coro(int high, int low, int self_idx) {
    uintptr_t ptr = (uintptr_t)((uintptr_t)high << 32 | (uintptr_t)(unsigned int)low);
    Scheduler *s = (Scheduler *)ptr;
    Coroutine *coro = &s->coros[self_idx];
    coro->cb(s, coro->arg);
    coro->done = true;
}

int spawn(Scheduler *s, CoroFn cb, void *arg) {
    if (!s || !cb) {
        fprintf(stderr, "spawn: NULL scheduler or callback\n");
        return -1;
    }

    /* pick a slot: recycle a finished one first, then grow high_water */
    int idx;
    if (s->free_count > 0) {
        idx = s->free_slots[--s->free_count];
    } else if (s->high_water < s->capacity) {
        idx = (int)s->high_water++;
    } else {
        fprintf(stderr, "spawn: capacity reached (%zu)\n", s->capacity);
        return -1;
    }

    Coroutine *coro = &s->coros[idx];

    int res = getcontext(&coro->context);
    if (res == -1) {
        fprintf(stderr, "getcontext failed\n");
        return -1;
    }

    size_t page = PAGE_SIZE;
    uint8_t *map = mmap(NULL, STACK_SIZE + page,
                        PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS,
                        -1, 0);
    if (map == MAP_FAILED) {
        fprintf(stderr, "mmap failed\n");
        return -1;
    }
    if (mprotect(map, page, PROT_NONE) == -1) {
        fprintf(stderr, "mprotect failed\n");
        munmap(map, STACK_SIZE + page);
        return -1;
    }

    coro->done     = false;
    coro->stack    = map;
    coro->cb       = cb;
    coro->arg      = arg;
    coro->self_idx = idx;
    coro->context.uc_stack.ss_size = STACK_SIZE;
    coro->context.uc_stack.ss_sp   = map + page;
    coro->context.uc_link          = &s->loop_ctx;

    uintptr_t ptr = (uintptr_t)s;
    makecontext(&coro->context, (void (*)())do_coro, 3,
                (int)(ptr >> 32), (int)(ptr & 0xFFFFFFFF), idx);

    rq_enqueue(s, idx);
    return 0;
}
