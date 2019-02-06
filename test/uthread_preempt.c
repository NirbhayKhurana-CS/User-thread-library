

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <uthread.h>

int thread3(void *arg) {
    printf("Entering thread3\n");
    return 0;
}

int thread2(void *arg) {
    printf("Entering thread2\n");
    uthread_create(thread3, NULL);
    while (1) {
        printf("In thread 2\n");
        sleep(0.5);
    }
    return 0;
}

int thread1(void* arg)
{
    printf("Entering thread1\n");
    uthread_create(thread2, NULL);
    // printf("in thread1 printing queue\n");
    // uthread_printQueue();
    // printf("in thread1 end printing queue\n");
    while (1) {
        printf("In thread 1\n");
        sleep(0.5);
    }
	printf("I am thread 1!\n");
    printf("End of thread 1\n");
	return 0;
}

int main(void)
{
	// uthread_t tid;
	uthread_create(thread1, NULL);
	// uthread_join(tid, NULL);
    while (1) {
        printf("In thread 0\n");
        sleep(0.5);
    }
	printf("end of main\n");
	return 0;
}
