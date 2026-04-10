CC     := clang
CFLAGS := -Wall -Wextra -g -Iinclude/ -D_XOPEN_SOURCE=700 -MMD -MP
SFLAGS := $(CFLAGS) -fsanitize=address,undefined

LIB_OBJS := bin/coro.o bin/scheduler.o

.PHONY: all build test sanitize clean bin

all: build

build: bin/main

test: bin/test_coro
	./bin/test_coro

sanitize: bin/main_san bin/test_coro_san
	./bin/test_coro_san

# --- normal build ---

bin/main: bin/main.o $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

bin/main.o: examples/main.c include/zync.h | bin
	$(CC) $(CFLAGS) -c -o $@ $<

bin/coro.o: src/coro.c src/internal.h include/zync.h | bin
	$(CC) $(CFLAGS) -c -o $@ $<

bin/scheduler.o: src/scheduler.c src/internal.h include/zync.h | bin
	$(CC) $(CFLAGS) -c -o $@ $<

bin/test_coro.o: tests/test_coro.c include/zync.h | bin
	$(CC) $(CFLAGS) -c -o $@ $<

bin/test_coro: bin/test_coro.o $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# --- sanitizer build ---

bin/coro_san.o: src/coro.c src/internal.h include/zync.h | bin
	$(CC) $(SFLAGS) -c -o $@ $<

bin/scheduler_san.o: src/scheduler.c src/internal.h include/zync.h | bin
	$(CC) $(SFLAGS) -c -o $@ $<

bin/main_san.o: examples/main.c include/zync.h | bin
	$(CC) $(SFLAGS) -c -o $@ $<

bin/test_coro_san.o: tests/test_coro.c include/zync.h | bin
	$(CC) $(SFLAGS) -c -o $@ $<

bin/main_san: bin/main_san.o bin/coro_san.o bin/scheduler_san.o
	$(CC) $(SFLAGS) -o $@ $^

bin/test_coro_san: bin/test_coro_san.o bin/coro_san.o bin/scheduler_san.o
	$(CC) $(SFLAGS) -o $@ $^

# --- utils ---

bin:
	mkdir -p bin

-include $(wildcard bin/*.d)

clean:
	rm -rf bin
