#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <uthread.h>
#include <time.h>

/*
 * Test file for preempt. Detail of how this file can test the functionality of
 * preemption can be found in our report.
 */
int thread3(void *arg) {
    printf("Entering thread 3\n");
    printf("End of thread 3\n");
    return 0;
}

int thread2(void *arg) {
    printf("Entering thread 2\n");
    uthread_create(thread3, NULL);

    clock_t start, diff;
    // static count is set to prevent keep printing a lot of test strings in loop.
    static int count = 0;
    int sec = 0, trigger = 5;
    start = clock();

    /*
     * Set a timer to break the loop after 5 second. During this loop,
     * it should break the loop and switch to thread 1, and then switch back.
     */
    do {
        diff = clock() - start;
        sec = diff / CLOCKS_PER_SEC;
        if (count == 0 && sec == 2) {
            printf("I am in thread 2, time is: %d \n", sec);
            count++;
        }
    } while (sec < trigger);

    printf("End of thread 2, time is: %d\n", sec);
    return 0;

}

int thread1(void* arg) {
    printf("Entering thread 1\n");
    uthread_create(thread2, NULL);
    // static count is set to prevent keep printing a lot of test strings in loop.
    static int count = 0;
    clock_t start, diff;
    int sec = 0, trigger = 8;
    start = clock();

    /*
     * Set a timer to break the loop after 8 second.
     * During this time interval, it should break the loop and switch to thread 2.
     */
    do {
        diff = clock() - start;
        // printf("diff is %ld\n",diff);
        sec = diff / CLOCKS_PER_SEC;
        if (sec == 4 && count == 0) {
            printf("I am in thread 1, time is: %d \n", sec);
            count++;
        }
    } while (sec < trigger);

    printf("End of thread 1, time is %d\n", sec);
	return 0;
}

int main(void) {
    printf("begin main\n");
	uthread_t tid;
	tid = uthread_create(thread1, NULL);
	uthread_join(tid, NULL);
	printf("end of main\n");
	return 0;
}
