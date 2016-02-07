#!/usr/bin/make -f

CFLAGS=-Wall -Wextra -std=c11
CPPFLAGS=-Wall -Wextra -std=c++11

all: detect_syscalls aplusb

detect_syscalls: src/detect_syscalls.c
	gcc ${CFLAGS} src/detect_syscalls.c -o detect_syscalls

aplusb: src/aplusb.cpp
	g++ ${CPPFLAGS} src/aplusb.cpp -o aplusb

clean:
	rm -f aplusb detect_syscalls res/aplusb.out

.PHONY:   main clean
.DEFAULT: main
