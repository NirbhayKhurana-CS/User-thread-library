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
#define MAX_THREAD_NUM 1000

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
    uthread_t joinedThreadArray[MAX_THREAD_NUM];
    int joinedThreadArrayLength;
    Thread *runningThread;
};

static ThreadControl s;
static ThreadControl *threadControl = &s;

/*
 * count is the number of totoal threads, including main threads.
 * It is static to this file.
 */
static int count = 0;

#define self threadControl->runningThread

bool threadIsJoined(uthread_t threadId) {
    for (int i = 0; i < threadControl->joinedThreadArrayLength; i++) {
        if (threadControl->joinedThreadArray[i] == threadId) {
            return true;
        }
    }
    return false;
}

void joinedThreadArrayInsert(uthread_t threadId) {
    if (threadIsJoined(threadId)) {
        printf("Error: trying to insert a joined thread into array\n");
    }
    threadControl->joinedThreadArray[threadControl->joinedThreadArrayLength] = threadId;
    threadControl->joinedThreadArrayLength++;
}

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
    threadControl->joinedThreadArrayLength = 0;
    Thread *mainThread = malloc(sizeof(Thread));
    if (mainThread == NULL) {
        return -1;
    }
    mainThread->child = NULL;
    // mainThread->joined = false;
    void *mainStack = uthread_ctx_alloc_stack();
    mainThread->context = malloc(sizeof(uthread_ctx_t));


    mainThread->context->uc_stack.ss_sp = mainStack;
	mainThread->context->uc_stack.ss_size = UTHREAD_STACK_SIZE;

    mainThread->threadId = 0;
    mainThread->state = RUNNING;
    threadControl->runningThread = mainThread;
    printf("main thread is: %p \n", mainThread);
    printf("main thread context is: %p \n", mainThread->context);
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
    // thread->joined = false;
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
    printf("right now running thread about to exit is: %p, its threadId is: %d\n",
        threadControl->runningThread, (int)self->threadId);
    printf("number of thread exit is: %d \n", countExit);
    queue_print(threadControl->readyQueue);

    // Get next ready thread t.
	Thread *t = malloc(sizeof(Thread));
    queue_dequeue(threadControl->readyQueue,(void **)&t);
    printf("t, the next ready thread, when dequeued is: %p, its threadId is %d, its context is: %p\n",
        t, (int)t->threadId, t->context);

    t->state = RUNNING;

    // temp was running thread, and it is about to be zombie.
    Thread *temp = malloc(sizeof(Thread));
    temp = threadControl->runningThread;
    threadControl->runningThread = t;
    // printf("t after assign is: %p\n", t-);

    // Set self return value.
    temp->selfRetval = retval;
    temp->state = ZOMBIE;

    // Move parent thread from blockedQueue to readyQueue.
    Thread *parent = malloc(sizeof(Thread));
    Thread *beforeItr = parent;
    queue_iterate(threadControl->blockedQueue, &findParent, &temp->threadId, (void **)&parent);
    if (parent == beforeItr) {
        printf("parent not found\n");
        printf("t thread context when dequeed is: %p \n", t->context);
        uthread_ctx_switch(temp->context, threadControl->runningThread->context);
    }
    queue_delete(threadControl->blockedQueue, parent);
    queue_enqueue(threadControl->readyQueue, parent);

    // Move myself to zombieQueue.
    queue_enqueue(threadControl->zombieQueue, temp);

    // Everything is settled. Switch context.
    uthread_ctx_switch(temp->context, threadControl->runningThread->context);
}

int findChild(void* data, void* tid) {
    if (((Thread *)data)->threadId == *(uthread_t *)tid) {
        return 1;
    }
    return 0;
}

int uthread_join(uthread_t tid, int *retval) {
    if (tid == uthread_self() || tid == 0) {
        printf("Trying to join self or main thread\n");
        return -1;
    }


    while (true) {

        // Set and find child.
        Thread *childThread = malloc(sizeof(Thread));
        Thread *beforeItr = childThread;

        // If child is in readyQueue, move myself to blockedQueue.
        queue_iterate(threadControl->readyQueue, &findChild, &tid, (void **)&childThread);
        if (childThread != beforeItr) {
            if (threadIsJoined(childThread->threadId)) {
                printf("in readyQueue child already joined\n");
                return -1;
            }
            printf("child is in readyQueue\n");
            // Connect child to parent.
            threadControl->runningThread->child = childThread;
            printf("my child is: %p, id is %d\n", childThread, childThread->threadId);
            Thread *t1 = threadControl->runningThread;
            Thread *t2 = malloc(sizeof(Thread));
            int dequeueRetval = queue_dequeue(threadControl->readyQueue, (void **)&t2);
            // If successful dequeued.
            if (dequeueRetval == -1) {
                printf("ERROR: nothing in dequeue\n");
            }
            t2->state = RUNNING;
            threadControl->runningThread = t2;
            t1->state = BLOCKED;
            queue_enqueue(threadControl->blockedQueue, t1);
            printf("I just enqueue because child in readyQueue t1 is: %p, its id is: %d\n", t1, t1->threadId);
            uthread_ctx_switch(t1->context, t2->context);
            break;
        }

        // If child is in blockedQueue, move myself to blockedQueue.
        queue_iterate(threadControl->blockedQueue, &findChild, &tid, (void **)&childThread);
        if (childThread != beforeItr) {
            if (threadIsJoined(childThread->threadId)) {
                printf("in blockedQueue child already joined\n");
                return -1;
            }
            // Connect child to parent.
            printf("child is in blockedQueue\n");
            queue_print(threadControl->blockedQueue);
            threadControl->runningThread->child = childThread;
            Thread *t1 = threadControl->runningThread;
            Thread *t2 = malloc(sizeof(Thread));
            int dequeueRetval = queue_dequeue(threadControl->readyQueue, (void **)&t2);
            if (dequeueRetval != -1) {
                t2->state = RUNNING;
                threadControl->runningThread = t2;
            }
            t1->state = BLOCKED;
            queue_enqueue(threadControl->blockedQueue, t1);
            printf("I just enqueue because child in blockedQueue t1 is: %p, its id is: %d\n", t1, t1->threadId);
            uthread_ctx_switch(t1->context, t2->context);
            break;
        }

        // If child is in zombieQueue, Collect data and free child.
        queue_iterate(threadControl->zombieQueue, &findChild, &tid, (void **)&childThread);
        if (childThread != beforeItr) {
            if (threadIsJoined(childThread->threadId)) {
                printf("in zombieQueue child already joined\n");
                return -1;
            }
            printf("child is in zombieQueue\n");
            *retval = threadControl->runningThread->child->selfRetval;
            joinedThreadArrayInsert(childThread->threadId);
            queue_delete(threadControl->zombieQueue, threadControl->runningThread->child);
            printf("thread about to be destroyed is %p, its id is: %d \n", childThread, (int)childThread->threadId);
            printf("stack about to be destroyed is: %p\n", self->child->context->uc_stack.ss_sp);
            uthread_ctx_destroy_stack(self->child->context->uc_stack.ss_sp);
            printf("context about to be destroyed is: %p\n", self->child->context);
            free(threadControl->runningThread->child->context);
            printf("thread about to be destroyed is: %p\n", threadControl->runningThread->child);
            free(threadControl->runningThread->child);
            break;
        }

        else {
            printf("In uthread_join, tid not found in iterate\n");
            return -1;
        }
        // threadControl->runningThread->child = childThread;









    }

    return 0;
}
