#include <stdio.h>  // Remove when submit.
#include <stdlib.h>  // Remove when submit.
#include <assert.h>
#include "queue.h"

static int find_item(void *data, void *arg)
{
    int *a = (int*)data;
    int match = (int)(long)arg;
    if (*a == match){
		return 1;
	}
    return 0;
}

struct Qnode;
struct queue;
typedef struct queue* queue_t;
int main() {
    assert(queue_destroy(NULL) == -1);
    assert(queue_enqueue(NULL, NULL) == -1);
	queue_t p;
	p = queue_create();
	int a = 0;
	int b = 1;
	int c = 2;
	int d = 3;
	int e = 4;
	int f;
	// int g = 5;
	void *fp = &f;
	void **fpp = &fp;
	queue_enqueue(p,&a);
	queue_enqueue(p,&b);
	queue_enqueue(p,&c);
	queue_enqueue(p,&d);
	queue_enqueue(p,&e);
	queue_print(p);
	queue_dequeue(p,fpp);
	queue_delete(p,&c);
	printf("fpp is %d\n", *(int*)fp);
	queue_print(p);
	int len = queue_length(p);
	printf("length is %d\n", len);
	queue_iterate(p, &find_item, (void*)4, fpp);
	printf("fpp is %d\n", *(int*)fp);
    return 0;

}
