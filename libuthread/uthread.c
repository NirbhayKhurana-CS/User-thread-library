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

/* TODO Phase 2 */
#define MAX_THREAD 32768

enum status{running, ready, blocked, zombie};

// Forward decleration because two struct includes each other's pointer.
typedef struct Thread Thread;
typedef struct ThreadControl ThreadControl;

struct Thread {
    uthread_t threadId;
    int state;
    void *stack;
    uthread_ctx_t *context;
    ThreadControl *controlAddr;
};

struct ThreadControl {
    int threadCount; // ThreadCount doesn't include the main thread
    Thread *threadArray[MAX_THREAD];
};

static ThreadControl *threadControl;

/*
 * count is the number of totoal threads, including main threads.
 * It is static to this file.
 */
static int count = 0;

void uthread_yield(void)
{
	/* TODO Phase 2 */
}

uthread_t uthread_self(void)
{
	/* TODO Phase 2 */
}

int first_uthread_create(ThreadControl *threadControl) {
    threadControl->threadCount = 0;
    Thread *firstThread = malloc(sizeof(Thread));
    if (firstThread == NULL) {
        return -1;
    }
    firstThread->stack = uthread_ctx_alloc_stack();
    firstThread->threadId = 0;
    // TODO state
    threadControl->threadArray[0] = firstThread;
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
        if (first_uthread_create(threadControl) == -1) {
            return -1;
        }
    }
    count++;
    thread->stack = uthread_ctx_alloc_stack();
    ucontext_t *uctx = malloc(sizeof(ucontext_t));
    if (uctx == NULL) {
        return -1;
    }
    int ctxInitRetval = uthread_ctx_init(uctx, thread->stack, func, arg);
    if (ctxInitRetval == -1) {
        return -1;
    }
    thread->context = uctx;
    thread->threadId = count;
    threadControl->threadCount = count;
    threadControl->threadArray[count] = thread;
    return thread->threadId;
}

void uthread_exit(int retval)
{
	/* TODO Phase 2 */
}

int uthread_join(uthread_t tid, int *retval)
{
	/* TODO Phase 2 */
	/* TODO Phase 3 */
}
