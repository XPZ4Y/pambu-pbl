CC = gcc
CFLAGS = -Wall -Wextra -O2

scheduler: scheduler.c
	$(CC) $(CFLAGS) scheduler.c -o scheduler

run: scheduler
	./scheduler

clean:
	rm -f scheduler

.PHONY: run clean
