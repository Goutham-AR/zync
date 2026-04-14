#include "zync.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define PASS(name) fprintf(stderr, "PASS  %s\n", name)

/* ------------------------------------------------------------------ helpers */

static int g_order[64];
static int g_order_count;

static void record(int val) {
    g_order[g_order_count++] = val;
}

static void reset_order(void) {
    g_order_count = 0;
    memset(g_order, 0, sizeof(g_order));
}

/* ------------------------------------------------------------------ tests */

/* 1. Normal completion — coroutine runs and finishes without yielding */
static void simple_cb(Scheduler* s, void* arg) {
    (void)s;
    record(*(int*)arg);
}

static void test_normal_completion(void) {
    Scheduler s;
    assert(!scheduler_init(&s, 4));

    int vals[3] = {10, 20, 30};
    reset_order();
    assert(!spawn(&s, simple_cb, &vals[0], 0, "simple-0"));
    assert(!spawn(&s, simple_cb, &vals[1], 0, "simple-1"));
    assert(!spawn(&s, simple_cb, &vals[2], 0, "simple-2"));
    assert(!run_loop(&s));

    assert(g_order_count == 3);
    assert(g_order[0] == 10);
    assert(g_order[1] == 20);
    assert(g_order[2] == 30);

    scheduler_destroy(&s);
    PASS("normal_completion");
}

/* 2. Yield/resume ordering — two coroutines interleave round-robin */
static void interleave_cb(Scheduler* s, void* arg) {
    int id = *(int*)arg;
    for (int i = 0; i < 3; ++i) {
        record(id * 10 + i);
        assert(!yield(s));
    }
}

static void test_yield_ordering(void) {
    Scheduler s;
    assert(!scheduler_init(&s, 4));

    int a = 1, b = 2;
    reset_order();
    assert(!spawn(&s, interleave_cb, &a, 0, "interleave-a"));
    assert(!spawn(&s, interleave_cb, &b, 0, "interleave-b"));
    assert(!run_loop(&s));

    /* expected: 10,20, 11,21, 12,22 */
    assert(g_order_count == 6);
    assert(g_order[0] == 10);
    assert(g_order[1] == 20);
    assert(g_order[2] == 11);
    assert(g_order[3] == 21);
    assert(g_order[4] == 12);
    assert(g_order[5] == 22);

    scheduler_destroy(&s);
    PASS("yield_ordering");
}

/* 3. Spawn-at-capacity — returns -1 when all slots are occupied */
static void long_cb(Scheduler* s, void* arg) {
    (void)arg;
    yield(s);
}

static void test_spawn_at_capacity(void) {
    Scheduler s;
    assert(!scheduler_init(&s, 2));

    int a = 1, b = 2, c = 3;
    /* fill both slots — coroutines haven't finished yet */
    assert(!spawn(&s, long_cb, &a, 0, NULL));
    assert(!spawn(&s, long_cb, &b, 0, NULL));

    /* third spawn must fail while slots are occupied */
    int ret = spawn(&s, long_cb, &c, 0, NULL);
    assert(ret == -1);

    run_loop(&s);
    scheduler_destroy(&s);
    PASS("spawn_at_capacity");
}

/* 4. Slot recycling — after coroutines finish, new ones reuse their slots */
static void noop_cb(Scheduler* s, void* arg) {
    (void)s;
    (void)arg;
}

static void test_slot_recycling(void) {
    Scheduler s;
    assert(!scheduler_init(&s, 2));

    int a = 0, b = 0;
    assert(!spawn(&s, noop_cb, &a, 0, "first-a"));
    assert(!spawn(&s, noop_cb, &b, 0, "first-b"));
    assert(!run_loop(&s));

    /* both slots freed — high_water stays at 2 */
    assert(s.free_count == 2);
    assert(s.high_water == 2);

    /* spawn again — must reuse, not exceed capacity */
    assert(!spawn(&s, noop_cb, &a, 0, "reuse-a"));
    assert(!spawn(&s, noop_cb, &b, 0, "reuse-b"));
    assert(s.high_water == 2);

    assert(!run_loop(&s));
    scheduler_destroy(&s);
    PASS("slot_recycling");
}

/* 5. Custom stack size — spawn with explicit size, verify it runs correctly */
static void stack_size_cb(Scheduler* s, void* arg) {
    (void)s;
    record(*(int*)arg);
}

static void test_custom_stack_size(void) {
    Scheduler s;
    assert(!scheduler_init(&s, 4));

    int val = 42;
    reset_order();
    /* use a larger-than-default stack */
    assert(!spawn(&s, stack_size_cb, &val, 128 * 1024, "big-stack"));
    assert(!run_loop(&s));

    assert(g_order_count == 1);
    assert(g_order[0] == 42);
    assert(s.coros[0].stack_size == 128 * 1024);

    scheduler_destroy(&s);
    PASS("custom_stack_size");
}

/* 6. Coroutine name — verify name is stored and visible via scheduler_dump */
static void named_cb(Scheduler* s, void* arg) {
    (void)s;
    (void)arg;
}

static void test_coroutine_name(void) {
    Scheduler s;
    assert(!scheduler_init(&s, 4));

    assert(!spawn(&s, named_cb, NULL, 0, "my-coro"));
    assert(strcmp(s.coros[0].name, "my-coro") == 0);

    /* anonymous coroutine */
    assert(!spawn(&s, named_cb, NULL, 0, NULL));
    assert(s.coros[1].name == NULL);

    run_loop(&s);
    scheduler_destroy(&s);
    PASS("coroutine_name");
}

/* ------------------------------------------------------------------ main */

int main(void) {
    fprintf(stderr, "Running coroutine tests...\n");

    test_normal_completion();
    test_yield_ordering();
    test_spawn_at_capacity();
    test_slot_recycling();
    test_custom_stack_size();
    test_coroutine_name();

    fprintf(stderr, "All tests passed.\n");
    return 0;
}
