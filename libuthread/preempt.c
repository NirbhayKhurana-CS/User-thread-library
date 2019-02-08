#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "preempt.h"
#include "uthread.h"

/*
 * Frequency of preemption
 * 100Hz is 100 times per second
 */
#define HZ 100
#define MSEC 10000

// Set sigaction
static struct sigaction sa;
static struct itimerval timer;

void preempt_disable(void) {
    sigprocmask(SIG_BLOCK, &sa.sa_mask, NULL);
}

void preempt_enable(void) {
    sigprocmask(SIG_UNBLOCK, &sa.sa_mask, NULL);
}

void handler(int sig) {
    if (sig == SIGVTALRM) {
        uthread_yield();
    }
}

void preempt_start(void) {
    // Set signal handler.
    sa.sa_handler = handler;
    // Initialize signal mask.
    sigemptyset(&sa.sa_mask);
    // Set the flags
    sa.sa_flags = 0;
    // Set sugaction.
    sigaction(SIGVTALRM, &sa, NULL);

    // Set up the timer.
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = MSEC;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = MSEC;

    setitimer(ITIMER_VIRTUAL, &timer, NULL);
}
