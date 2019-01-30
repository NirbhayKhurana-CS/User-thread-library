# Target library
# lib := libuthread.a
#
# all: $(lib)

default: libuthread.a

queue.o: queue.c queue.h
	gcc -Wall -Werror -c -o queue.o queue.c

libuthread.a: queue.o
	ar rcs libuthread.a queue.o
