#pragma once

#include "../include/zync.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

/* MAP_ANONYMOUS is hidden under _XOPEN_SOURCE=700 on macOS */
#ifndef MAP_ANONYMOUS
#  define MAP_ANONYMOUS 0x1000
#endif

#define PAGE_SIZE  ((size_t)sysconf(_SC_PAGE_SIZE))

void coro_free(Coroutine *c);

static inline void rq_enqueue(Scheduler *s, int idx) {
    s->ready_q[s->rq_tail] = idx;
    s->rq_tail = (s->rq_tail + 1) % s->capacity;
    ++s->rq_count;
}

static inline int rq_dequeue(Scheduler *s) {
    int idx = s->ready_q[s->rq_head];
    s->rq_head = (s->rq_head + 1) % s->capacity;
    --s->rq_count;
    return idx;
}
