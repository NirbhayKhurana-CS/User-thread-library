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

    /*
     * If a thread is collected, we insert threadId to this array,
     * this array will help to check whether we are joining a joined thread.
     */
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

// yield put running thread to readyQueue, and bring up next readyQueue.
// If no thread is in readyQueue, we run the first thread in blockedQueue.
void uthread_yield(void) {
	Thread *t1 = threadControl->runningThread;
    Thread *t2 = malloc(sizeof(Thread));
    int dequeueRetval = queue_dequeue(threadControl->readyQueue, (void **)&t2);
    if (dequeueRetval == -1) {
        int blockedRetval = queue_dequeue(threadControl->blockedQueue,(void **)&t2);
        // If no thread in blockedQueue.
        if (blockedRetval == -1) {
            return;
        }
        // Some thread in blockedQueue. We don't need to write else here.
    }
    t2->state = RUNNING;
    threadControl->runningThread = t2;
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
    // Initialize threadControl.
    threadControl->readyQueue = queue_create();
    threadControl->zombieQueue = queue_create();
    threadControl->blockedQueue = queue_create();
    threadControl->joinedThreadArrayLength = 0;
    Thread *mainThread = malloc(sizeof(Thread));
    if (mainThread == NULL) {
        return -1;
    }
    mainThread->child = NULL;
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
    thread->context = malloc(sizeof(uthread_ctx_t));
    if (thread->context == NULL) {
        return -1;
    }
    int ctxInitRetval = uthread_ctx_init(thread->context, threadStack, func, arg);
    if (ctxInitRetval == -1) {
        return -1;
    }
    thread->controlAddr = threadControl;
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
	Thread *t = malloc(sizeof(Thread));
    int readyRetval = queue_dequeue(threadControl->readyQueue,(void **)&t);
    // If no thread in readyQueue.
    if (readyRetval == -1) {
        int blockedRetval = queue_dequeue(threadControl->blockedQueue,(void **)&t);
        // If no thread in blockedQueue.
        if (blockedRetval == -1) {
            return;
        }
        // Some thread in blockedQueue, we immediately popup first one and let it run.
        else {
            t->state = RUNNING;

            // temp was running thread, and it is about to be zombie.
            Thread *temp = malloc(sizeof(Thread));
            temp = threadControl->runningThread;
            threadControl->runningThread = t;
            // printf("t after assign is: %p\n", t-);

            // Set self return value.
            temp->selfRetval = retval;
            temp->state = ZOMBIE;

            // Move myself to zombieQueue.
            queue_enqueue(threadControl->zombieQueue, temp);

            // Everything is settled. Switch context.
            uthread_ctx_switch(temp->context, threadControl->runningThread->context);
            return;
        }
    }
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
        uthread_ctx_switch(temp->context, threadControl->runningThread->context);
    }
    queue_delete(threadControl->blockedQueue, parent);
    queue_enqueue(threadControl->readyQueue, parent);

    // Move myself to zombieQueue.
    queue_enqueue(threadControl->zombieQueue, temp);

    // Everything is settled. Switch context.
    uthread_ctx_switch(temp->context, threadControl->runningThread->context);
    return;
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
            // Connect child to parent.
            threadControl->runningThread->child = childThread;
            Thread *t1 = threadControl->runningThread;
            Thread *t2 = malloc(sizeof(Thread));
            int dequeueRetval = queue_dequeue(threadControl->readyQueue, (void **)&t2);
            // If successful dequeued.
            if (dequeueRetval == -1) {
                printf("ERROR: nothing in dequeue but it should be\n");
            }
            t2->state = RUNNING;
            threadControl->runningThread = t2;
            t1->state = BLOCKED;
            queue_enqueue(threadControl->blockedQueue, t1);
            uthread_ctx_switch(t1->context, t2->context);
            continue;
        }

        // If child is in blockedQueue, move myself to blockedQueue.
        queue_iterate(threadControl->blockedQueue, &findChild, &tid, (void **)&childThread);
        if (childThread != beforeItr) {
            if (threadIsJoined(childThread->threadId)) {
                printf("in blockedQueue child already joined\n");
                return -1;
            }
            // Connect child to parent.
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
            uthread_ctx_switch(t1->context, t2->context);
            continue;
        }

        // If child is in zombieQueue, Collect data and free child.
        queue_iterate(threadControl->zombieQueue, &findChild, &tid, (void **)&childThread);
        if (childThread != beforeItr) {
            if (threadIsJoined(childThread->threadId)) {
                printf("in zombieQueue child already joined\n");
                return -1;
            }
            if (retval != NULL) {
                *retval = threadControl->runningThread->child->selfRetval;
            }
            joinedThreadArrayInsert(childThread->threadId);
            queue_delete(threadControl->zombieQueue, threadControl->runningThread->child);
            uthread_ctx_destroy_stack(self->child->context->uc_stack.ss_sp);
            free(threadControl->runningThread->child->context);
            free(threadControl->runningThread->child);
            break;
        }

        else {
            printf("In uthread_join, tid not found in iterate\n");
            return -1;
        }
    }
    return 0;
}
