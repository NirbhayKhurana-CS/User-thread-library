#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>  // Remove when submit.
#include "queue.h"

typedef struct {
	void * key;
	struct Qnode *next;
} Qnode;

typedef struct {
	int length;
	struct Qnode *front;
	struct Qnode *back;
} Queue;

queue_t queue_create(void) {
	queue_t q = malloc(sizeof(Queue));
	q->front = NULL;
	q->back = NULL;
	q->length = 0;
	return q;
}

int queue_destroy(queue_t queue) {

}

int queue_enqueue(queue_t queue, void *data) {
	if (data == NULL || queue == NULL) {
		return -1;
	}
	Qnode *newNode = malloc(sizeof(Qnode));
	if (newNode == NULL) {
		printf("malloc node goes wrong\n");
		return -1;
	}
	newNode->key = data;
	newNode->next = NULL;
	if (queue->length == 0) {
		queue->front = newNode;
		queue->back = newNode;
	}
	else {
		queue->back->next = newNode;
		queue->back = newNode;
	}
	queue->length++;
	return 0;
}

int queue_dequeue(queue_t queue, void **data) {
	if (data == NULL || queue == NULL || queue->length == 0) {
		return -1;
	}
	if (queue->length == 1) {
		*data = queue->front->key;
		free(queue->front);
		queue->front = NULL;
		queue->back = NULL;
	}
	else {
		struct Qnode *temp = queue->front;
		*data = queue->front->key;
		queue->front = queue->front->next;
		free(temp);
	}
	queue->length--;
	return 0;
}

int queue_delete(queue_t queue, void *data) {
	if (data == NULL || queue == NULL || queue->length == 0) {
		return -1;
	}
	Qnode *temp = queue->front;
	// The element we want to delete is front.
	if (data == temp->key) {
		if (queue->length == 1) {
			free(queue->front);
			queue->front = NULL;
			queue->back = NULL;
		}
		else {
			queue->front = queue->front->next;
			free(temp);
		}
		queue->length--;
		return 0;
	}
	// The element we want to delete is not front.
	while (temp->next != NULL) {
		if (temp->next->key == data) {
			// Delete next node.
			temp->next = temp->next->next;
			return 0;
		}
		temp = temp->next;
	}
	return -1;
}

int queue_iterate(queue_t queue, queue_func_t func, void *arg, void **data) {
	/* TODO Phase 1 */
}

int queue_length(queue_t queue) {
	/* TODO Phase 1 */
}

// Remove when submit.
void main() {
	struct Queue *p = queue_create();
	int a = 0;
	int b = 1;
	int c = 2;
	int d = 3;
	int e = 4;
	int f;
	int *fp = &f;
	int **fpp = &fp;
	queue_enqueue(p,&a);
	queue_enqueue(p,&b);
	queue_enqueue(p,&c);
	queue_enqueue(p,&d);
	queue_enqueue(p,&e);
	queue_dequeue(p,fpp);
	queue_delete(p,&c);
	struct Qnode* temp = p->front;
	while(temp != NULL) {
		printf("item %d \n", *(int*)temp->key);
		temp = temp->next;
	}

}
