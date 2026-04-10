# g_async

A minimal cooperative coroutine scheduler in C, built on POSIX `ucontext`.

Coroutines are spawned onto a fixed-size scheduler that runs a round-robin loop,
resuming each coroutine in turn. A coroutine calls `yield()` to voluntarily hand
control back to the scheduler.

> Work in progress.
