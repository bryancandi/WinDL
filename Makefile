# Makefile for WinDL

CC ?= cc
CFLAGS ?= -Wall -Wextra -Wpedantic
LDFLAGS ?= -lwininet

all: windl

windl: windl.c
	$(CC) $(CFLAGS) windl.c -o windl $(LDFLAGS)

clean:
	rm -f windl.exe
