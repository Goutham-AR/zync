CC := clang
CFLAGS := -Wall -Wextra -g -Iinclude/ -D_XOPEN_SOURCE=700 -MMD -MP

.PHONY: all build bin clean

all: build

build: main

main: bin/main.o bin/coro.o bin/scheduler.o
	$(CC) $(CFLAGS) -o bin/main bin/main.o bin/coro.o bin/scheduler.o

bin/main.o: examples/main.c include/zync.h | bin
	$(CC) $(CFLAGS) -c -o bin/main.o examples/main.c

bin/coro.o: src/coro.c src/internal.h include/zync.h | bin
	$(CC) $(CFLAGS) -c -o bin/coro.o src/coro.c

bin/scheduler.o: src/scheduler.c src/internal.h include/zync.h | bin
	$(CC) $(CFLAGS) -c -o bin/scheduler.o src/scheduler.c

bin:
	mkdir -p bin

-include $(wildcard bin/*.d)

clean:
	rm -rf bin
