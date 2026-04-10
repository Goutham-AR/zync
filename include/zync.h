#pragma once

#include <ucontext.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * Default number of coroutine slots when no capacity is specified.
 * Pass a different value to scheduler_init() to override.
 */
#define CORO_DEFAULT_CAPACITY   20

/*
 * Default stack size (bytes) used when spawn() is called with stack_size == 0.
 */
#define CORO_DEFAULT_STACK_SIZE (64 * 1024)

typedef struct Scheduler Scheduler;

/*
 * Coroutine callback type.
 *
 * @param s        The scheduler that owns this coroutine.
 * @param userdata Caller-supplied context pointer passed to spawn().
 */
typedef void (*CoroFn)(Scheduler *s, void *userdata);

/*
 * Internal per-coroutine state. Do not modify fields directly.
 * All fields are managed by spawn(), yield(), and coro_free().
 */
typedef struct {
    ucontext_t   context;
    bool         done;
    uint8_t     *stack;        /* base of mmap region (includes guard page) */
    size_t       stack_size;   /* usable stack size in bytes (excl. guard)  */
    CoroFn       cb;
    void        *arg;
    const char  *name;         /* optional debug name; caller owns the string */
    int          self_idx;
} Coroutine;

/*
 * Scheduler state. Initialise with scheduler_init(); tear down with
 * scheduler_destroy(). Do not copy or move after initialisation.
 */
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

/*
 * Initialise a scheduler.
 *
 * @param s        Pointer to an uninitialised Scheduler.
 * @param capacity Maximum number of concurrent coroutines.
 * @return 0 on success, -1 on invalid arguments or allocation failure.
 */
int  scheduler_init(Scheduler *s, size_t capacity);

/*
 * Destroy a scheduler, freeing all coroutine stacks and internal arrays.
 * Safe to call even if some coroutines have not finished.
 *
 * @param s Pointer to an initialised Scheduler.
 */
void scheduler_destroy(Scheduler *s);

/*
 * Print the current state of every coroutine slot to stderr.
 * Useful for debugging hangs or unexpected scheduler behaviour.
 *
 * @param s Pointer to an initialised Scheduler.
 */
void scheduler_dump(const Scheduler *s);

/*
 * Spawn a new coroutine.
 *
 * @param s          The owning scheduler.
 * @param cb         Coroutine entry point. Must not be NULL.
 * @param arg        Opaque pointer forwarded to cb as userdata.
 * @param stack_size Stack size in bytes. Pass 0 to use CORO_DEFAULT_STACK_SIZE.
 * @param name       Optional human-readable name for debugging. May be NULL.
 *                   The pointer must remain valid for the coroutine's lifetime.
 * @return 0 on success, -1 on error (NULL args, capacity reached, alloc fail).
 */
int spawn(Scheduler *s, CoroFn cb, void *arg, size_t stack_size, const char *name);

/*
 * Yield execution back to the scheduler from within a coroutine.
 * The coroutine is re-enqueued and will be resumed on a future scheduler tick.
 *
 * Must only be called from within a running coroutine (not from main or
 * from a thread that is not currently being scheduled).
 *
 * @param s The owning scheduler.
 * @return 0 on success, -1 on error (NULL scheduler, called outside coroutine).
 */
int yield(Scheduler *s);

/*
 * Run all queued coroutines to completion.
 *
 * Repeatedly dequeues and resumes coroutines until the ready-queue is empty.
 * Finished coroutine slots are recycled automatically.
 *
 * @param s The owning scheduler.
 * @return 0 on success, -1 if a context switch fails.
 */
int run_loop(Scheduler *s);
