CC ?= cc
CFLAGS ?= -Wall -Wextra -O0

all: thread_demo

thread_demo: thread_demo.c
	$(CC) $(CFLAGS) -pthread thread_demo.c -o thread_demo

clean:
	rm -f thread_demo
