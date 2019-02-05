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
    bool joined; // Self return value is joined.
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
    int dequeueRetval = queue_dequeue(threadControl->readyQueue, (void **)&t2);
    if (dequeueRetval != -1) {
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
    mainThread->joined = false;
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
    thread->joined = false;
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
    static int countExit = 0;
    countExit++;
    printf("number of thread exit is: %d \n", countExit);
    queue_print(threadControl->readyQueue);
    printf("right now running thread is: %p\n", threadControl->runningThread);
    printf("its thread id is: %d \n", (int)threadControl->runningThread->threadId);
    // Get next ready thread t.
	Thread *t;
    queue_dequeue(threadControl->readyQueue,(void **)&t);
    printf("t is: %p\n", t);
    printf("t context when dequeued is: %p\n", t->context);
    printf("t threadid when dequeed is: %d \n", (int)t->threadId);

    t->state = RUNNING;

    // temp was running thread, and it is about to be zombie.
    Thread *temp = threadControl->runningThread;
    threadControl->runningThread = t;
    // printf("t after assign is: %p\n", t-);

    // Set self return value.
    temp->selfRetval = retval;
    temp->joined = true;
    temp->state = ZOMBIE;

    // Move parent thread from blockedQueue to readyQueue.
    Thread *parent = malloc(sizeof(Thread));
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

int findChild(void* data, void* tid) {
    if ((Thread *)data->threadId == *(uthread_t *)tid) {
        return 1;
    }
    return 0;
}

int uthread_join(uthread_t tid, int *retval) {
    if (tid == uthread_self || tid == 0) {
        printf("Trying to join self or main thread\n");
        return -1;
    }
    // Set and find child.
    Thread *childThread;
    int iterateReadyRetval =
        queue_iterate(threadControl->readyQueue, &findChild, &tid, (void **)&childThread);
    int iterateZombieRetval =
        queue_iterate(threadControl->zombieQueue, &findChild, &tid, (void **)&childThread);
    int iterateBlockedRetval =
        queue_iterate(threadControl->blockedQueue, &findChild, &tid, (void **)&childThread);

    // // Errro check.
    // int countZero = 0;
    // if (interateReadyRetval == 0) {
    //     countZero++;
    // }
    // if (iterateZombieRetval == 0) {
    //     countZero++
    // }
    // if (iterateBlockedRetval == 0) {
    //     countZero++;
    // }
    // if (countZero > 1) {
    //     printf("ERROR: child exists in multiple queue\n");
    // }

    // If myself does not have child.
    if (interateReadyRetval + iterateZombieRetval + iterateBlockedRetval == 3) {
        uthread_yield();
    }

    if (childThread->joined) {
        return -1;
    }
    // Connect child to parent.
    threadControl->runningThread->child = childThread;

    // If child is in readyQueue or blockedQueue, move myself to blockedQueue.
    if (iterateReadyRetval == 0 || iterateBlockedRetval == 0) {
        Thread *t1 = threadControl->runningThread;
        Thread *t2 = malloc(sizeof(Thread));
        int dequeueRetval = queue_dequeue(threadControl->readyQueue, (void **)&t2);
        if (dequeueRetval != -1) {
            t2->state = RUNNING;
            threadControl->runningThread = t2;
        }
        t1->state = BLOCKED;
        queue_enqueue(threadControl->blockedQueue, t1);
        uthread_ctx_switch(t1->context, t2->context);
    }

    // If child is in zombieQueue, Collect data and free child.
    if (iterateZombieRetval == 0) {
        if (joined) {
            /* code */
        }
        *retval = threadControl->runningThread->child->selfRetval;
        queue_delete(threadControl->zombieQueue, threadControl->runningThread->child);
        free(threadControl->runningThread->child);
    }






    if (interateRetval == 1) {
        printf("In uthread_join, tid not found in iterate\n");
        return -1;
    }
    threadControl->runningThread->child = childThread;



    return 0;
}
