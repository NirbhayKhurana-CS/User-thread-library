
# Project overview
In this project, we implement the following objects files for the libuthread.a
library.
1. Queue implementation
2. Uthread implementation
3. Preemption implementation
We also create two additional test cases to test our queue and preemption
1. uthread_preempt
2. test_queue

## Queue implementation
The function of the queue library is to assist the uthread library. In the uthread
 library, we store each state in a queue, so that we can access the data we stored
 easily. The important part when writing the queue library is to use the most
 efficient structure. At first, we used array to construct the queue, and then
 we found that it is very easy to make mistake to keep track of the array’s front
 and end index especially when deleting a specific item in the middle of the queue.
 We need to move half of the array frontwards. Therefore, we use linkedlist. The
 advantage of linkedlist is that when deleting an element at middle, we simply let
 the last element’s pointer points to the next element. Besides, we can easily
 delete or add node to the current linkedlist by using its front and back pointer.

## Uthread implementation
There are four major parts on the uthread.c file: uthread_create, uthread_yield,
uthread_join and uthread_exit. Each of them require us to understand how the process
execution flow works. Since in a process, there are multiple different states threads,
we store the threads in the ready state, zombie state and blocked state in separate
queues in threadControl, and we represent them as the readyQueue, zombieQueue, and
blockedQueue. We do not put the running state into a queue since there can be only
one thread running at a time for user-level thread. Below we will talk about our
implementation process for each specific function.
#### Uthread_create
In this part we use a separate function to initialize the main thread. Apart from
the fact that main thread does not need us to allocate extra stacks for it, we want
to initialize the main control, threadControl to regulate every thread. The
threadControl includes three queues (readyQueue, zombieQueue, blockedQueue) and
an array to store the already been joined thread’s ID. This array will help us to
check whether user is joining an already joined thread in error check.
After we initialize the threadControl, we intialize the stack and context for the
new thread and put it in the readyQueue.
#### Uthread_yield
Since the yield function ask us to yield the current running thread, our intuition
is to put the current running thread at the end of the readyQueue and pop the first
thread in readyQueue and put it on the running state.
#### Uthread_join
There are two things we understand in order to implement this part.
1. We have to use a infinite loop to loop through the joined funcion, and the loop
will only break when the child thread is in zombieQueue waiting to be joined. The
reason for doing that is we want to keep waiting for the child to become zombie if
it is not finished, for example, still in readyQueue or blockedQueue.
2. There are three situations when a parent wants to join his child. When we search
for child thread in blockedQueue and readyQueue, we need to connect the searched
child with its parent. There is no need to do so in the zombieQueue because in
uthread_exit, if a thread has a parent, then it will set the return value and move
itself to zombieQueue. If it is an orphan like in phase 4, it will not come into
zombieQueue, it will return.
    1. If child thread is in readyQueue, we move the current running thread to
    blockedQueue.Then we dequeue readyQueue, and put the result in the running
    state. In this situation, since child is in readyQueue, then the dequeue must
    succeed.
    2. If child thread is in blockedQueue, we move the current running thread to
    blockedQueue.Then we pop readyQueue, and put the result in the running state.
    If there is nothing in readyQueue, we simply return.
    3. If child is in zombieQueue, we collect data and free the child thread and
    break out the loop, indicating we are done with one join.
#### Uthread_exit
In context.c, this function is executed when the return value is available. The
assignment prompt says
>When a thread exits, its resources (such as its TCB) are not automatically freed.
It must first be “joined” by another thread, which can then (or not) collect the
return value of the dead thread.

It means all thread that is being joined should have a parent when reaching this
state. In addition, even though the name of the function is called exit, we actually
doesn’t free our thread in this state. Instead, we need to put thread that reaches
this function into zombieQueue, and we will only free those thread once its parent
is running again. If we free the memory now, the return value will be forever lost
and later in join, parent would fail to collect it. The process flow of this function
is described below.
First we move the current running thread into zombieQueue indicating that it is done.
Then, we move its parent from blockedQueue to readyQueue showing that its parent can
now start running. Then we pop from readyQueue and set the thread to running state
to keep the program running.
There are two special situations that should return directly.
1. When we reach the end of main, which does not have a parent thread, we need to
free and end the program immediately.
2. When a thread is not being joined, we should also pop the readyQueue and get
the next running thread.


## Preemption implementation
The logic of preemption is that, if a thread receive a signal, it should yield and
let the next available thread take the CPU to run. Holding this idea, we create a
signal handler `sigaction` type sa and a `itimerval` type timer. The signal handler
object is initialized with a handler function, signal mask and a flag with 0 as
default. The whole signal handler will put to start when we are creating the main
thread.
#### Handler function
The handler function is activated when it receives the signal `SIGVTALRM` and the
thread that is running the handler function will yield to next available thread.
#### Signal mask
The mask is set with signal `SIGVTALRM`. When a thread is initialized with preemption
enabled, the signal in sa.sa_mask will be unblocked so the thread will receive it
and yield. When we want to do context switch or manipulating threadControl’s property
such as runningThread, we don’t want thread 1 put a thread X to run then thread 2
put a thread Y to run, which leads to conflict. So during these situations, we
disable the preemption by blocking the signal set in sa.sa_mask, so the running
thread will not be disturbed by timer alarm.


## Test files

#### uthread_hello.c and uthread_yield.c
We modify the hello and yield test files by adding printing statements at the end
of their main functions. We do so to ensure that after a thread exits and is joined,
we come back to the main thread.
#### test_queue.c
In this file, apart from making sure that a NULL queue will return -1, we insert
and delete elements and print out the element that returns 1 in iterating function.
In the earlier version, we also add another method in queue.h to print out each
queue element to make sure that they are in proper sequence, but since modifying
header file is not allowed in submission, we remove it.
#### uthread_preempt.c
In this file, in order to truly prove that threads are switching because of
preemption not join, we only have one join function in main thread. After main
thread creates thread 1, thread 1 creates thread 2 and start this while loop for
8 second. During this 8 second, if our preemption works, and it does, it should
switch to execute thread 2, and in thread 2, in addition to creating thread 3, it
also has a while loop which last for 5 second. So the behavior should be during
thread 1 and thread 2’s while loop, thread 3 will be executed and finished. Then,
in thread 2 it will print when the time reaches 2 second. After 2 second, the
handler successfully prove that at this time thread 1’s printing statement is
executed by showing thread 1 is at second 4. Then at second 5, thread 2’s loop
is finished, at last thread 1 and main is finished. The ability to print out
statements in different threads according to time without using join proves that
we are actually switching between different threads.

```
jackied@ad3.ucdavis.edu@pc29 ~/ecs-150-p2/test (master) $ ./uthread_preempt.x
begin main
Entering thread 1
Entering thread 2
Entering thread 3
End of thread 3
I am in thread 2, time is: 2
I am in thread 1, time is: 4
End of thread 2, time is: 5
End of thread 1, time is 8
end of main
jackied@ad3.ucdavis.edu@pc29 ~/ecs-150-p2/test (master) $
```

## Miscellaneous

#### Static variable
We rely on static variables in this program for two situation. The first one is
to know when we are firstly creating a main thread. We need a counter that can
check how many times the uthread_create is called. If it is called for the first
time, then we know we want to initialize the main thread and threadControl. The
next situation we use it will be discussed along with the next section, sleep and
clock.
#### Sleep clock and printing statement
When we wanted to test the preemption at the beginning, the first idea we have
was to let a thread sleep for 2 second, during which the running thread has to
switch for multiple times. The testing program keeps failing to switch to other
threads when current thread is sleeping. It is not until the due date that professor
explained to me that sleep since even though this thread is calling sleep, it will
make the whole process to sleep. I ask why in Linux man page it writes
>sleep() makes the calling thread sleep until seconds seconds have elapsed or a
signal arrives which is not ignored.

The professor does not know. So we finally stop using sleep to test our code, and
turns to clock to make the while loop last for a certain amount of time.
However, if we simply put a printing statement inside each thread’s while loop,
it will print a lot, and use a lot of time to display in terminal. The way we manage
to overcome this difficulty is set a static variable. In the while loop, we increment
this static variable, and we print only if the variable is a certain value and the
loop goes for a certain time. By this, we successfully prevent printing statements
overflowing the terminal output and prove that we are indeed switching between
different threads.
