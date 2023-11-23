#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>
#include <malloc.h>

#include "storage.h"

void *monitor(void *arg) {
	storage_t *s = (storage_t *)arg;

	printf("monitor: [%d %d]\n", getpid(), getppid());

	while (1) {
		storage_print_stats(s);
		sleep(1);
	}

	return NULL;
}

storage_t* storage_init(int max_count) {
	int err;
	storage_t* s = (storage_t*)malloc(sizeof(storage_t));
	printf("%ld\n", sizeof(s));

	s->first = NULL;
	s->max_count = max_count;
	s->count = 0;

	s->add_attempts = s->get_attempts = 0;
	s->add_count = s->get_count = 0;

	err = pthread_create(&s->monitor_tid, NULL, monitor, s);
	if (err) {
		printf("storage_init: pthread_create() failed: %s\n", strerror(err));
		abort();
	}
	
	printf("storage_init: storage inited\n");

	return s;
}

void storage_destroy(storage_t *s) {
	char* val;
	if (s->count == 0) {
		return;
	}
	printf("storage_destroy: begin destroy\n");
	while (s->first->next != NULL) {
		val = storage_get(s);
		printf("destroyed snode->value = %s\n", val);
	}

	val = storage_get(s);
	printf("destroyed snode->value = %s\n", val);
}

int storage_add(storage_t *s, char* val) {
	s->add_attempts++;

	assert(s->count <= s->max_count);

	if (s->count == s->max_count)
		return 0;

	snode_t *new = malloc(sizeof(snode_t));
	if (!new) {
		printf("Cannot allocate memory for new node\n");
		abort();
	}

	strcpy(new->val, val);
	new->next = NULL;

	if (!s->first)
		s->first = new;
	else {
		new->next = s->first;
		s->first = new;
	}

	s->count++;
	s->add_count++;

	return 1;
}

// Get first snode from storage and delete it
char* storage_get(storage_t *s) {
	s->get_attempts++;

	assert(s->count >= 0);

	if (s->count == 0)
		return 0;

	snode_t *tmp = s->first;

	char* val = malloc(sizeof(tmp->val));

	strcpy(val, tmp->val);
	s->first = s->first->next;

	free(tmp);
	s->count--;
	s->get_count++;

	return val;
}

void storage_print_stats(storage_t *s) {
	printf("storage stats: current size %d; attempts: (add: %ld; get: %ld; add-get: %ld); counts (add: %ld; get: %ld; add-get: %ld)\n",
		s->count,
		s->add_attempts, s->get_attempts, s->add_attempts - s->get_attempts,
		s->add_count, s->get_count, s->add_count - s->get_count);
}

void print_storage(storage_t* storage) {
    if (storage->count == 0) {
        printf("print storage: Storage is empty\n");
        return;
    }
	snode_t* cur = storage->first;
    for (int i = 0; i < storage->count; i++) {
        printf("storage[%d]->val = %s\n", i, cur->val);
		cur = cur->next;
    }
}

snode_t* find_prev(storage_t* storage, snode_t* node) {
	//printf("find_prev: node->val = %s\n", node->val); 
	snode_t* cur;
	if (storage->count == 0) {
		//printf("find_prev: storage is empty\n");
		return NULL;
	}

	if (storage->first!=NULL) {
		//printf("find_prev: storage->first!=NULL\n");
		cur = storage->first;
	}

	for (int i = 0; i < storage->count; i++) {
		if (cur->next == node) {
			return cur;
		}
		cur = cur->next;
	}

	return NULL;
}


int swap(storage_t* storage, snode_t* node_1, snode_t* node_2) {
	// head->node_1->node_2->next
	if(node_1 == storage->first) {
		node_1->next = node_2->next;
		snode_t* tmp = node_1;
		storage->first = node_2;
		node_2->next = tmp;
	}

	else {
		snode_t* head_node = find_prev(storage, node_1);
		if (head_node == NULL) {
		printf("swap: cannot find head_node\n");
		return 0;
		}
		node_1->next = node_2->next;
		snode_t* tmp = node_1;
		head_node->next = node_2;
		node_2->next = tmp;
	}
	
	return 1;
}

