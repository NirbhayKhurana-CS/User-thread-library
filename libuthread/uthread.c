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
#define UTHREAD_STACK_SIZE 32768

enum status { RUNNING, READY, BLOCKED, ZOMBIE };
// Forward decleration because two struct includes each other's pointer.
typedef struct Thread Thread;
typedef struct ThreadControl ThreadControl;

struct Thread {
    uthread_t threadId;
    int selfRetval; // Self return value.
    int state;
    uthread_ctx_t *context;
    ThreadControl *controlAddr;
    Thread *child;
    bool valid; // Self return value is valid.
};

struct ThreadControl {
    // All three queues store thread pointer.
    queue_t readyQueue;
    queue_t zombieQueue;
    queue_t blockedQueue;
    Thread *runningThread;
};

static ThreadControl s;
static ThreadControl *threadControl = &s;

/*
 * count is the number of totoal threads, including main threads.
 * It is static to this file.
 */
static int count = 0;

void uthread_yield(void) {
	Thread *t1 = threadControl->runningThread;
    Thread *t2 = malloc(sizeof(Thread));
    int exitReady = queue_dequeue(threadControl->readyQueue, (void **)&t2);
    if (exitReady != -1) {
        t2->state = RUNNING;
        threadControl->runningThread = t2;
    }
    t1->state = READY;
    queue_enqueue(threadControl->readyQueue, t1);
    uthread_ctx_switch(t1->context, t2->context);
}

uthread_t uthread_self(void) {
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
    threadControl->zombieQueue = queue_create();
    threadControl->blockedQueue = queue_create();
    Thread *mainThread = malloc(sizeof(Thread));
    if (mainThread == NULL) {
        return -1;
    }
    mainThread->child = NULL;
    mainThread->valid = false;
    void *mainStack = uthread_ctx_alloc_stack();
    mainThread->context = malloc(sizeof(uthread_ctx_t));


    mainThread->context->uc_stack.ss_sp = mainStack;
	mainThread->context->uc_stack.ss_size = UTHREAD_STACK_SIZE;

    mainThread->threadId = 0;
    mainThread->state = RUNNING;
    threadControl->runningThread = mainThread;
    return 0;
}

int uthread_create(uthread_func_t func, void *arg) {
    // Check if we are creating main thread.
    if (count == 0) {
        // threadControl = malloc(sizeof(ThreadControl));
        if (threadControl == NULL) {
            return -1;
        }
        if (main_uthread_create(threadControl) == -1) {
            return -1;
        }
    }
    count++;
    // This is the new thread to register.
    Thread *thread = malloc(sizeof(Thread));
    if (thread == NULL) {
        return -1;
    }
    thread->valid = false;
    thread->child = NULL;
    void *threadStack = uthread_ctx_alloc_stack();
    // uthread_ctx_t *uctx = malloc(sizeof(uthread_ctx_t));
    thread->context = malloc(sizeof(uthread_ctx_t));
    if (thread->context == NULL) {
        return -1;
    }
    int ctxInitRetval = uthread_ctx_init(thread->context, threadStack, func, arg);
    if (ctxInitRetval == -1) {
        return -1;
    }
    thread->controlAddr = threadControl;
    // thread->context = (uthread_ctx_t *)malloc(sizeof(uthread_ctx_t));
    thread->threadId = count;
    thread->state = READY;
    queue_enqueue(threadControl->readyQueue, thread);
    return thread->threadId;
}

int findParent(void* thread, void* childId) {
    if (((Thread *)thread)->child->threadId == *(uthread_t *)childId) {
        return 1;
    }
    return 0;
}

void uthread_exit(int retval) {
    // Get next ready thread t.
	Thread *t;
    queue_dequeue(threadControl->readyQueue,(void **)&t);
    printf("t context when dequeued is: %p\n", t->context);

    t->state = RUNNING;

    // temp was running thread, and it is about to be zombie.
    Thread *temp = threadControl->runningThread;
    threadControl->runningThread = t;
    // printf("t after assign is: %p\n", t-);

    // Set self return value.
    temp->selfRetval = retval;
    temp->valid = true;
    temp->state = ZOMBIE;

    // Move parent thread from blockedQueue to readyQueue.
    Thread *parent;
    int iterateRetval = queue_iterate(threadControl->blockedQueue, &findParent, &temp->threadId, (void **)&parent);
    if (iterateRetval == -1) {
        printf("iterateRetval goes wrong\n");
    }
    queue_delete(threadControl->blockedQueue, parent);
    queue_enqueue(threadControl->readyQueue, parent);

    // Move myself to zombieQueue.
    queue_enqueue(threadControl->zombieQueue, temp);

    // Everything is settled. Switch context.
    printf("t->context is: %p\n", t->context);
    printf("runningThread context is: %p\n", threadControl->runningThread->context);
    uthread_ctx_switch(temp->context, threadControl->runningThread->context);
}

int uthread_join(uthread_t tid, int *retval) {
    while (true) {
        if (queue_length(threadControl->readyQueue) == 0) {
            break;
        }
        else {
            uthread_yield();
        }
    }
    return 0;
}
