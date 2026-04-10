#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>

#include "coro.h"

void coro_free(Coroutine* c) {
    // assert(c->done && c->stack);
    if (c->done && c->stack) {
        free(c->stack);
    }
    c->stack = NULL;
}
