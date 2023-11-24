#define _GNU_SOURCE
#include <pthread.h>
#include <assert.h>
#include <malloc.h>
#include <pthread.h>

#include "storage.h"

void set_cpu(int n) {
	int err;
	cpu_set_t cpuset;
	pthread_t tid = pthread_self();

	CPU_ZERO(&cpuset);
	CPU_SET(n, &cpuset);

	// Function for attaching thread to the cpu core. cpu_set_t
	//  - struct representing the set of available processor cores.
	err = pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpuset);
	if (err) {
		printf("set_cpu: pthread_setaffinity failed for cpu %d\n", n);
		return;
	}
}

void *monitor(void *arg) {
	storage_t *s = (storage_t *)arg;

	set_cpu(0);

	printf("monitor: [%d %d %ld]\n", getpid(), getppid(), pthread_self());

	while (1) {
		storage_print_stats(s);
		sleep(1);
	}

	return NULL;
}

storage_t* storage_init(int max_count, int sync_option) {
	int err;
	storage_t* s = (storage_t*)malloc(sizeof(storage_t));
	s->sync_option = sync_option;

	
	snode_t* head = (snode_t*)malloc(sizeof(snode_t));
	if (sync_option == SPINLOCK) {
		if (pthread_spin_init(&head->spinlock, PTHREAD_PROCESS_SHARED) != 0) {
			printf("Failed to initialize spinlock\n");
        	abort();
		}
	}
	else {
		if (pthread_mutex_init(&head->mutex, NULL) != 0) {
			printf("Failed to initialize mutex\n");
        	abort();
		}
		
	}
	

	s->head = head;
	s->head->next = NULL;
	
	s->max_count = max_count;
	s->count = 0;

	s->add_attempts = s->get_attempts = 0;
	s->add_count = s->get_count = 0;

	/*err = pthread_create(&s->monitor_tid, NULL, monitor, s);
	if (err) {
		printf("storage_init: pthread_create() failed: %s\n", strerror(err));
		abort();
	}*/

	return s;
}

void storage_destroy(storage_t *s) {
	char* val;
	if (s->count == 0) {
		free(s->head);
		return;
	}

	while (s->head->next != NULL) {
		val = storage_get(s);
		printf("destroyed snode->value = %s\n", val);
	}
	free(s->head);
	printf("destroyed head\n");
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

	if (s->sync_option == SPINLOCK) {
		if (pthread_spin_init(&new->spinlock, PTHREAD_PROCESS_SHARED) != 0) {
			printf("Failed to initialize spinlock\n");
        	abort();
		}
	}

	else {
		if (pthread_mutex_init(&new->mutex, NULL) != 0) {
			printf("Failed to initialize mutex\n");
        	abort();
		}
	}
	

	strcpy(new->val, val);
	new->next = NULL;

	new->next = s->head->next;
	s->head->next = new;

	s->count++;
	s->add_count++;

	return 1;
}

// Get first snode from storage and delete it
char* storage_get(storage_t *s) {
	s->get_attempts++;

	assert(s->count >= 0);

	if (s->count == 0) {
		return 0;
	}
		
	snode_t *tmp = s->head;

	char* val = malloc(sizeof(tmp->val));

	strcpy(val, tmp->val);
	s->head = s->head->next;

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
	snode_t* cur = storage->head->next;
    for (int i = 0; i < storage->count; i++) {
        printf("storage[%d]->val = %s\n", i, cur->val);
		cur = cur->next;
    }
}


int swap(storage_t* storage, snode_t* head_node, snode_t* node_1, snode_t* node_2) {
	// head_node->node_1->node_2->next
	/*pthread_spin_lock(&head_node->spinlock);
	pthread_spin_lock(&node_1->spinlock);
	pthread_spin_lock(&node_2->spinlock);*/
	

	if (storage->debug_mode) {
		printf("\n\n\n\n");
		printf("swap: head = %s\n", head_node->val);
		printf("swap: node_1 = %s\n", node_1->val);
		printf("swap: node_2 = %s\n", node_2->val);
	}

	node_1->next = node_2->next;
	snode_t* tmp = node_1;
	head_node->next = node_2;
	node_2->next = tmp;

	if (storage->debug_mode) {
		print_storage(storage);
		printf("\n\n\n\n");
	}

	
	/*pthread_spin_unlock(&head_node->spinlock);
	pthread_spin_unlock(&node_1->spinlock);
	pthread_spin_unlock(&node_2->spinlock);
	*/

	return 1;
}

