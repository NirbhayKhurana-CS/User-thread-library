#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "context.h"
#include "preempt.h"
#include "queue.h"
#include "uthread.h"

#include <stdbool.h>
#include <limits.h>
enum status { RUNNING, READY, BLOCKED, ZOMBIE };

// Forward decleration because two struct includes each other's pointer.
typedef struct Thread Thread;
typedef struct ThreadControl ThreadControl;

struct Thread {
    uthread_t threadId;
    int state;
    uthread_ctx_t *context;
    ThreadControl *controlAddr;
};

struct ThreadControl {
    queue_t readyQueue; // readyQueue stores Thread pointer.
    Thread *runningThread;
};

static ThreadControl *threadControl;

/*
 * count is the number of totoal threads, including main threads.
 * It is static to this file.
 */
static int count = 0;

void uthread_yield(void) {
	Thread *t1 = threadControl->runningThread;
    Thread *t2 = malloc(sizeof(Thread));
    queue_dequeue(threadControl->readyQueue, (void **)&t2);
    t1->state = READY;
    queue_enqueue(threadControl->readyQueue, t1);
    t2->state = RUNNING;
    threadControl->runningThread = t2;
    uthread_ctx_switch(t1->context, t2->context);
}

uthread_t uthread_self(void)
{
    if (threadControl == NULL || threadControl->runningThread == NULL) {
        printf("Something wrong in uthread_self\n");
        exit(1);
    }
    if (threadControl->runningThread->threadId >= USHRT_MAX) {
        printf("TID value overflow\n");
        exit(1);
    }
	return threadControl->runningThread->threadId;
}

int main_uthread_create(ThreadControl *threadControl) {
    threadControl->readyQueue = queue_create();
    Thread *mainThread = malloc(sizeof(Thread));
    if (mainThread == NULL) {
        return -1;
    }
    mainThread->threadId = 0;
    mainThread->state = RUNNING;
    threadControl->runningThread = mainThread;
    return 0;
}

int uthread_create(uthread_func_t func, void *arg) {
    // This is the new thread to register.
    Thread *thread = malloc(sizeof(Thread));
    if (thread == NULL) {
        return -1;
    }
    if (count == 0) {
        // ThreadControl *threadControl = malloc(sizeof(ThreadControl));
        if (threadControl == NULL) {
            return -1;
        }
        if (main_uthread_create(threadControl) == -1) {
            return -1;
        }
    }
    count++;
    void *threadStack = uthread_ctx_alloc_stack();
    ucontext_t *uctx = malloc(sizeof(ucontext_t));
    if (uctx == NULL) {
        return -1;
    }
    int ctxInitRetval = uthread_ctx_init(uctx, threadStack, func, arg);
    if (ctxInitRetval == -1) {
        return -1;
    }
    thread->controlAddr = threadControl;
    thread->context = uctx;
    thread->threadId = count;
    thread->state = READY;
    queue_enqueue(threadControl->readyQueue, thread);
    return thread->threadId;
}

void uthread_exit(int retval)
{
	/* TODO Phase 2 */
}

int uthread_join(uthread_t tid, int *retval) {
    while (true) {
        if (queue_length(threadControl->readyQueue) == 0) {
            break;
        }
        else {
            uthread_yield();
            break;
        }
    }
    return 0;
}
