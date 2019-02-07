

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <uthread.h>
#include <time.h>

// int msec = 0, trigger = 10000;
// clock_t before = clock();

int thread3(void *arg) {
    printf("Entering thread 3\n");
    printf("End of thread 3\n");
    return 0;
}

int thread2(void *arg) {
    printf("Entering thread 2\n");
    uthread_create(thread3, NULL);

    clock_t start, diff;
    // One second.
    int sec = 0, trigger = 5;
    start = clock();

    // Set a timer say for i > 2 sec, break the loop
    // Meaning it should swap to thread2 100 times
    do {
        diff = clock() - start;
        // printf("diff is %ld\n",diff);
        sec = diff / CLOCKS_PER_SEC;
        // uthread_printQueue();
        // printf("In thread 1\n");
        // Want to find how long do we spend in the loop;
        // Should be less than 0.01s

    } while (sec < trigger);

    printf("End of thread 2\n");
    return 0;

}

int thread1(void* arg)
{
    printf("Entering thread 1\n");
    uthread_create(thread2, NULL);

    static int count = 0;
    clock_t start, diff;
    // One second.
    int sec = 0, trigger = 8;
    start = clock();

    // Set a timer say for i > 2 sec, break the loop
    // Meaning it should swap to thread2 100 times
    do {
        diff = clock() - start;
        // printf("diff is %ld\n",diff);
        sec = diff / CLOCKS_PER_SEC;
        if (sec == 5 && count == 0) {
            printf("I am in thread 1, time is: %d \n", sec);
            count++;
        }
        // uthread_printQueue();
        // printf("In thread 1\n");
        // Want to find how long do we spend in the loop;
        // Should be less than 0.01s

    } while (sec < trigger);

    printf("End of thread 1, time is %d\n", sec);
	return 0;
}

int main(void)
{
    printf("begin main\n");
	uthread_t tid;
	tid = uthread_create(thread1, NULL);
	uthread_join(tid, NULL);
    // clock_t diff = clock() - before;
    // msec = diff * 1000 / CLOCKS_PER_SEC;
    // iterations++;
    // while (1) {
    //     printf("In thread 0\n");
    //     sleep(0.5);
    // }
	printf("end of main\n");
	return 0;
}
