CC := clang
CFLAGS := -Wall -Wextra -g


.PHONY: all build bin

all: build

build: main

main: bin/main.o bin/coro.o bin/scheduler.o
	$(CC) $(CFLAGS) -o bin/main bin/main.o bin/coro.o bin/scheduler.o

bin/main.o: src/main.c bin
	$(CC) $(CFLAGS) -c -o bin/main.o src/main.c

bin/coro.o: src/coro.c bin
	$(CC) $(CFLAGS) -c -o bin/coro.o src/coro.c

bin/scheduler.o: src/scheduler.c bin
	$(CC) $(CFLAGS) -c -o bin/scheduler.o src/scheduler.c

bin:
	mkdir -p bin
