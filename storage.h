#ifndef STORAGE_H
#define STORAGE_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

#define DESTROY_START -128

typedef struct _StorageNode {
        char val[100];
        struct _StorageNode* next;
        pthread_mutex_t sync;
} snode_t;

typedef struct _Storage {
	snode_t *first;

	pthread_t monitor_tid;

	int count;
	int max_count;

	// storage statistics
	long add_attempts;
	long get_attempts;
	long add_count;
	long get_count;

	volatile pthread_spinlock_t spinlock;
} storage_t;

storage_t* storage_init(int max_count);
void storage_destroy(storage_t *s);
int storage_add(storage_t *s, char* val);
char* storage_get(storage_t *s);
void storage_print_stats(storage_t *s);
void print_storage(storage_t* storage);
int swap(storage_t* storage, snode_t* head_node, snode_t* node_1, snode_t* node_2);

#endif //STORAGE_H

