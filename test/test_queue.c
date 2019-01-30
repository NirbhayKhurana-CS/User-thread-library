#include <stdio.h>  // Remove when submit.
#include <stdlib.h>  // Remove when submit.
#include <queue.h>
#include <assert.h>

static int test(void *data, void *arg) {
	int result;
	result = *(int*)data - *(int*)arg;
	if (result == -1) {
		return 1;
	}
	return 0;
}

static int find_item(void *data, void *arg)
{
    int *a = (int*)data;
    int match = (int)(long)arg;
    if (*a == match){
		return 1;
	}
    return 0;
}
int main() {
    assert(queue_destroy(NULL) == -1);
    assert(queue_enqueue(NULL, NULL) == -1);

	queue_t p = queue_create();
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
	queue_dequeue(p,fpp);
	queue_delete(p,&c);
	Qnode* temp = p->front;
	while(temp != NULL) {
		printf("item %d \n", *(int*)temp->key);
		temp = temp->next;
	}
	int len = queue_length(p);
	printf("length is %d\n", len);
	queue_func_t funcPtr = &find_item;
	queue_iterate(p, funcPtr, (void*)4, fpp);
	printf("fpp is %d\n", *(int*)fp);
    return 0;

}
