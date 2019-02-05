#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>  // Remove when submit.
#include "queue.h"

typedef struct Qnode {
	void *key;
	struct Qnode *next;
} Qnode;

typedef struct queue {
	int length;
	struct Qnode *front;
	struct Qnode *back;
} queue;

queue_t queue_create(void) {
	queue_t q = malloc(sizeof(queue));
	q->front = NULL;
	q->back = NULL;
	q->length = 0;
	return q;
}

int queue_destroy(queue_t queue) {
	if (queue == NULL || queue->length != 0) {
		return -1;
	}
	else {
		free(queue);
		return 0;
	}
}

int queue_enqueue(queue_t queue, void *data) {
	if (data == NULL || queue == NULL) {
		return -1;
	}
	Qnode *newNode = malloc(sizeof(Qnode));
	if (newNode == NULL) {
		// printf("malloc node goes wrong\n");
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
		queue->front = NULL;
		queue->back = NULL;
	}
	else {
		// struct Qnode *temp = queue->front;
		*data = queue->front->key;
		queue->front = queue->front->next;
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
			queue->front = NULL;
			queue->back = NULL;
		}
		else {
			queue->front = queue->front->next;
		}
		queue->length--;
		return 0;
	}
	// The element we want to delete is not front.
	while (temp->next != NULL) {
		if (temp->next->key == data) {
			// Delete next node.
			temp->next = temp->next->next;
			queue->length--;
			return 0;
		}
		temp = temp->next;
	}
	return -1;
}

int queue_iterate(queue_t queue, queue_func_t func, void *arg, void **data) {
	if (queue == NULL || func == NULL) {
		return -1;
	}
	Qnode *temp = queue->front;
	while(temp != NULL) {
		if (func(temp->key, arg) == 1) {
			if (data != NULL) {
				*data = temp->key;
			}
			return 0;
		}
		temp = temp->next;
	}
	printf("item not found in queue_iterate\n");
	return 1;
}

int queue_length(queue_t queue) {
	if (queue == NULL) {
		return -1;
	}
	else {
		return queue->length;
	}
}

// Remove when submit.
void queue_print(queue_t p) {
	printf("-----launch printing queue-----\n");
	if (p == NULL) {
		printf("this queue is null\n");
		return;
	}
	struct Qnode *temp;
	temp = p->front;
	while(temp != NULL) {
		printf("item %p \n", temp->key);
		temp = temp->next;
	}
	printf("-----finish printing queue-----\n");
}





// static int test(void *data, void *arg) {
// 	int result;
// 	result = *(int*)data - *(int*)arg;
// 	if (result == -1) {
// 		return 1;
// 	}
// 	return 0;
// }
//
// static int find_item(void *data, void *arg)
// {
//     int *a = (int*)data;
//     int match = (int)(long)arg;
//     if (*a == match){
// 		return 1;
// 	}
//     return 0;
// }

// void main() {
// 	queue_t p = queue_create();
// 	int a = 0;
// 	int b = 1;
// 	int c = 2;
// 	int d = 3;
// 	int e = 4;
// 	int f;
// 	int g = 5;
// 	void *fp = &f;
// 	void **fpp = &fp;
// 	queue_enqueue(p,&a);
// 	queue_enqueue(p,&b);
// 	queue_enqueue(p,&c);
// 	queue_enqueue(p,&d);
// 	queue_enqueue(p,&e);
// 	queue_dequeue(p,fpp);
// 	queue_delete(p,&c);
// 	struct Qnode* temp = p->front;
// 	while(temp != NULL) {
// 		printf("item %d \n", *(int*)temp->key);
// 		temp = temp->next;
// 	}
// 	int len = queue_length(p);
// 	printf("length is %d\n", len);
// 	queue_func_t funcPtr = &find_item;
// 	queue_iterate(p, funcPtr, (void*)4, fpp);
// 	printf("fpp is %d\n", *(int*)fp);
//
// }
