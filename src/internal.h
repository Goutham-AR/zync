#pragma once

#include "../include/zync.h"
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

/* MAP_ANONYMOUS is hidden under _XOPEN_SOURCE=700 on macOS */
#ifndef MAP_ANONYMOUS
#  define MAP_ANONYMOUS 0x1000
#endif

#define STACK_SIZE (64 * 1024)
#define PAGE_SIZE  ((size_t)sysconf(_SC_PAGE_SIZE))

void coro_free(Coroutine *c);
