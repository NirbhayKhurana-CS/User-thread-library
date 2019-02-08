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
    // Self return value.
    int selfRetval;
    int state;
    uthread_ctx_t *context;
    ThreadControl *controlAddr;
    Thread *child;
    // Self return value is joined.
    bool joined;
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

/*
 * We could just use s, but since we already start writting the code using
 * threadControl as a pointer, we do this extra step.
 */
static ThreadControl s;
static ThreadControl *threadControl = &s;

/*
 * count is the number of totoal threads, including main threads, abd We use
 * this variable to determine whether we are creating thread for the first time.
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

// Insert the joined thread into joinedThreadArray.
void joinedThreadArrayInsert(uthread_t threadId) {
    threadControl->joinedThreadArray[threadControl->joinedThreadArrayLength] = threadId;
    threadControl->joinedThreadArrayLength++;
}

/*
 * This function put running thread t1 to readyQueue, and bring up next
 * ready thread t2. If no thread is in readyQueue, we return.
 */
void uthread_yield(void) {
	Thread *t1 = threadControl->runningThread;
    Thread *t2 = malloc(sizeof(Thread));
    Thread *beforeItr = t2;
    int dequeueRetval = queue_dequeue(threadControl->readyQueue, (void **)&t2);
    if (dequeueRetval == -1) {
        return;
    }
    // If nothing in readyQueue.
    if (t2 == beforeItr) {
        return;
    }
    // If some thread is in readyQueue.
    t2->state = RUNNING;
    threadControl->runningThread = t2;
    t1->state = READY;
    queue_enqueue(threadControl->readyQueue, t1);
    preempt_disable();
    uthread_ctx_switch(t1->context, t2->context);
    preempt_enable();
}

uthread_t uthread_self(void) {
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

    // Initialize main thread.
    mainThread->child = NULL;
    void *mainStack = uthread_ctx_alloc_stack();
    mainThread->context = malloc(sizeof(uthread_ctx_t));
    mainThread->context->uc_stack.ss_sp = mainStack;
	mainThread->context->uc_stack.ss_size = UTHREAD_STACK_SIZE;
    mainThread->threadId = 0;
    mainThread->state = RUNNING;
    threadControl->runningThread = mainThread;
    preempt_start();
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
    // temp was running thread, and it is about to be zombie.
    Thread *temp = malloc(sizeof(Thread));
    preempt_disable();
    temp = threadControl->runningThread;
    // Set self return value.
    temp->selfRetval = retval;
    temp->state = ZOMBIE;
    preempt_enable();

    // Move myself to zombieQueue.
    queue_enqueue(threadControl->zombieQueue, temp);

    // Move parent thread from blockedQueue to readyQueue.
    Thread *parent = malloc(sizeof(Thread));
    Thread *beforeItr = parent;
    queue_iterate(threadControl->blockedQueue, &findParent, &temp->threadId, (void **)&parent);
    // If self has no parent in blockedQueue.
    if (parent == beforeItr) {
        // If nothing in readyQueue, we shall return.
        if (queue_length(threadControl->readyQueue) < 1) {
            return;
        }
        // If something in readyQueue, we get next ready thread t.
        else {
            Thread *t = malloc(sizeof(Thread));
            queue_dequeue(threadControl->readyQueue,(void **)&t);
            preempt_disable();
            threadControl->runningThread = t;
            t->state = RUNNING;
            preempt_enable();
            uthread_ctx_switch(temp->context, threadControl->runningThread->context);
            return;
        }
    }
    // If self has a parent in blockedQueue.
    queue_delete(threadControl->blockedQueue, parent);
    queue_enqueue(threadControl->readyQueue, parent);

    // Get next ready thread t.
    Thread *t = malloc(sizeof(Thread));
    queue_dequeue(threadControl->readyQueue,(void **)&t);

    preempt_disable();
    threadControl->runningThread = t;
    t->state = RUNNING;
    preempt_enable();

    // Everything is settled. Switch context.
    preempt_disable();
    uthread_ctx_switch(temp->context, threadControl->runningThread->context);
    preempt_enable();
    return;
}

int findChild(void* data, void* tid) {
    if (((Thread *)data)->threadId == *(uthread_t *)tid) {
        return 1;
    }
    return 0;
}

int uthread_join(uthread_t tid, int *retval) {
    // If a thread is trying to join myself or main thread.
    if (tid == uthread_self() || tid == 0) {
        return -1;
    }

    while (true) {
        // Set and find child.
        Thread *childThread = malloc(sizeof(Thread));
        Thread *beforeItr = childThread;

        /*
         * If child is in readyQueue, move myself to blockedQueue
         * and set running to the next in readyQueue.
         */
        queue_iterate(threadControl->readyQueue, &findChild, &tid, (void **)&childThread);
        if (childThread != beforeItr) {
            // If trying to join a joined child.
            if (threadIsJoined(childThread->threadId)) {
                return -1;
            }
            // Connect child to parent.
            preempt_disable();
            threadControl->runningThread->child = childThread;
            // Move myself to blockedQueue and popup next ready thread to run.
            Thread *t1 = threadControl->runningThread;
            Thread *t2 = malloc(sizeof(Thread));
            queue_dequeue(threadControl->readyQueue, (void **)&t2);
            t2->state = RUNNING;
            threadControl->runningThread = t2;
            t1->state = BLOCKED;
            queue_enqueue(threadControl->blockedQueue, t1);
            uthread_ctx_switch(t1->context, t2->context);
            preempt_enable();
            continue;
        }

        // If child is in blockedQueue, move myself to blockedQueue.
        queue_iterate(threadControl->blockedQueue, &findChild, &tid, (void **)&childThread);
        if (childThread != beforeItr) {
            if (threadIsJoined(childThread->threadId)) {
                return -1;
            }
            // Connect child to parent.
            threadControl->runningThread->child = childThread;
            preempt_disable();
            // Move myself to blockedQueue and popup next ready thread to run.
            Thread *t1 = threadControl->runningThread;
            Thread *t2 = malloc(sizeof(Thread));
            int dequeueRetval = queue_dequeue(threadControl->readyQueue, (void **)&t2);
            // If no thread is in readyQueue, we return.
            if (dequeueRetval == -1) {
                return 0;
            }
            t2->state = RUNNING;
            threadControl->runningThread = t2;
            t1->state = BLOCKED;
            queue_enqueue(threadControl->blockedQueue, t1);
            uthread_ctx_switch(t1->context, t2->context);
            preempt_enable();
            continue;
        }

        // If child is in zombieQueue, Collect data and free child.
        queue_iterate(threadControl->zombieQueue, &findChild, &tid, (void **)&childThread);
        if (childThread != beforeItr) {
            if (threadIsJoined(childThread->threadId)) {
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
            /*
             * If a thread finishes joining, and there is no more ready thread,
             * we shall destory the dynamic allocated queues.
             */
            if (queue_length(threadControl->readyQueue) == 0) {
                queue_destroy(threadControl->readyQueue);
                queue_destroy(threadControl->blockedQueue);
                queue_destroy(threadControl->zombieQueue);
            }
            break;
        }

        // If thread id cannot be found.
        else {
            return -1;
        }
    }
    return 0;
}
