# Target library
# lib := libuthread.a
#
# all: $(lib)

default: libuthread.a

queue.o: queue.c queue.h
	gcc -Wall -Werror -o queue.o queue.c -c

context.o: context.c context.h uthread.h preempt.h
	gcc -Wall -Werror -o context.o context.c -c

uthread.o: uthread.c uthread.h context.h queue.h
	gcc -Wall -Werror -o uthread.o uthread.c -c

preempt.o: preempt.c preempt.h uthread.h
	gcc -Wall -Werror -o preempt.o preempt.c -c

libuthread.a: queue.o uthread.o context.o preempt.o
	ar rcs libuthread.a queue.o uthread.o context.o preempt.o

clean:
	rm -f libuthread.a queue.o context.o uthread.o preempt.o
