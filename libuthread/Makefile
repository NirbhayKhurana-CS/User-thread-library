# Target library
# lib := libuthread.a
#
# all: $(lib)

default: libuthread.a

queue.o: queue.c queue.h
	gcc -Wall -Werror -o queue.o queue.c -c

context.o: context.c context.h uthread.c 
	gcc -Wall -Werror -o context.o context.c -c

uthread.o: uthread.c uthread.h
	gcc -Wall -Werror -o uthread.o uthread.c -c

libuthread.a: queue.o uthread.o
	ar rcs libuthread.a queue.o uthread.o context.o

clean:
	rm -f libuthread.a queue.o uthread.o context.o
