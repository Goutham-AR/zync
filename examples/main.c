#include "zync.h"
#include <assert.h>
#include <stdio.h>

static void counter(Scheduler *s, void *userdata) {
    int id = *(int *)userdata;
    for (int i = 0; i < 3; ++i) {
        printf("[%d] %d\n", id, i);
        assert(!yield(s));
    }
}

int main(void) {
    Scheduler scheduler;
    assert(!scheduler_init(&scheduler, CORO_DEFAULT_CAPACITY));

    int id1 = 1, id2 = 2, id3 = 3;
    assert(!spawn(&scheduler, counter, &id1, 0, "counter-1"));
    assert(!spawn(&scheduler, counter, &id2, 0, "counter-2"));
    assert(!spawn(&scheduler, counter, &id3, 0, "counter-3"));

    assert(!run_loop(&scheduler));

    scheduler_destroy(&scheduler);
    return 0;
}
