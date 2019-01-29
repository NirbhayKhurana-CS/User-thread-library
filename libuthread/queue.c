#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>  // Remove when submit.
#include "queue.h"

struct queue {
	int front, back, size;
    void* array[];
};

queue_t queue_create(void)
{
	queue_t queuePtr = malloc(sizeof(struct queue));
    queuePtr->front = 0;
    queuePtr->back = -1;
    queuePtr->size = 0;
    //queuePtr->array = malloc(0 * (sizeof(void*)));
    return queuePtr;
}

int queue_destroy(queue_t queue)
{
	if (queue == NULL || queue->size != 0) {
        return -1;
    }
    else {
        //printf("Goes into queue_destroy\n");
        //free(queue->array);
        //printf("freed array\n");
        free(queue);
        //printf("freed queue\n");
        return 0;
    }
}

int queue_enqueue(queue_t queue, void *data)
{
    if (queue == NULL || data == NULL) {
        return -1;
    }
    else {
        queue->array[queue->back+1] = data;
        // TODO retirn -1 memory allocation error when enqueing
        queue->back++;
        queue->size++;

        return 0;
    }

}

int queue_dequeue(queue_t queue, void **data)
{
    if (queue == NULL || data == NULL) {
        return -1;
    }
    else {
        printf("before address of pointer %p \n", data);
        // printf("before storing data is %d \n", *((int*)*data));
        data = &queue->array[queue->front];
        printf("dequeu value inside function is %d \n", *(int*)queue->array[queue->front]);
        printf("after storing data is %d \n", *((int*)*data));
        printf("after address of pointer %p \n", data);
        queue->array[queue->front] = NULL;
        queue->front++;
        queue->size--;

        return 0;
    }
}

int queue_delete(queue_t queue, void *data)
{
	/* TODO Phase 1 */
}

int queue_iterate(queue_t queue, queue_func_t func, void *arg, void **data)
{
	/* TODO Phase 1 */
}

int queue_length(queue_t queue)
{
	/* TODO Phase 1 */
}

// Remove when submit.
void main() {
    queue_t ptr = queue_create();
    int a = 3;
    int c = 5;
    int enqueueValue = queue_enqueue(ptr, &a);
    printf("enqueueValue is: %d \n", enqueueValue);
    queue_enqueue(ptr, &c);
    for (int i = ptr->front; i < ptr->back+1; i++) {
        printf("before dequeue ptr[i] is: %d \n", *(int*)ptr->array[i]);
    }

    // void **b;
    int b = 100;
    void *bp;
    bp = &b;
    void **bpp;
    bpp = &bp;
    int dequeueValue = queue_dequeue(ptr, bpp);
    printf("dequeued value b %p \n", bpp);
    // printf("dequeued value bp %d \n", (int)*bp);
    // printf("dequeued value bpp %d \n", *((int*)*bpp));
    // for (int i = ptr->front; i < ptr->back+1; i++) {
    //     printf("after dequeue ptr[i] is: %d \n", *(int*)ptr->array[i]);
    // }
    // printf("dequeue return is %d \n", dequeueValue);
    // int destroyValue = queue_destroy(ptr);
    // printf("destroyValue is: %d\n", destroyValue);
    return;

}
