#pragma once
#define _XOPEN_SOURCE
#include <ucontext.h>
#include <stdint.h>
#include <stdbool.h>

typedef void(*Callback)(int);

typedef struct {
    ucontext_t context;
    bool done;
    uint8_t* stack;
    Callback cb;
} Coroutine;

void coro_free(Coroutine* c);
