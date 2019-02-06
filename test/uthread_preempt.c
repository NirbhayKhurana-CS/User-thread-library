

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <uthread.h>

int thread2(void *arg) {
    printf("Entering thread2\n");
    printf("End of thread2\n");
    return 0;
}

int thread1(void* arg)
{
    printf("Entering thread1\n");
    uthread_create(thread2, NULL);
    // printf("in thread1 printing queue\n");
    // uthread_printQueue();
    // printf("in thread1 end printing queue\n");
    sleep(2);
	printf("I am thread 1!\n");
    printf("End of thread 1\n");
	return 0;
}

int main(void)
{
	uthread_t tid;
	tid = uthread_create(thread1, NULL);
	uthread_join(tid, NULL);
	printf("end of main\n");
	return 0;
}
