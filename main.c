#define _GNU_SOURCE
#include <stdio.h>
#include "storage.h"
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>

#define DESTROY_START -128
#define CPUSET_MASK_SIZE 16

#define DECREASE -1
#define INCREASE 1
#define EQUAL 0

int thread1_iter_count = 0;
int thread2_iter_count = 0;
int thread3_iter_count = 0;
int swap_attempts = 0;
int swapped = 0;


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

	printf("set_cpu: set cpu %d\n", n);
}

void initialize_random_generator() {
	srand(time(NULL));
}

int generateRandomBit() {
    int result = rand() % 2;
	printf("generateRandomBit: result = %d\n", result);
	return result;
}

int countDigits(int number) {
	if (number == 0) {
		return 1;
	}

	int digitCount = 0;
	while (number != 0) {
		number /= 10;
		++digitCount;
	}

	return digitCount;
}

char* intToString(int number) {
	char* result = (char*)malloc(countDigits(number) * sizeof(char));

	sprintf(result, "%d", number);

	return result;
}

char* createString() {
	const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	int length = rand()%100;

	if (length == 0) {
		length++;
	}

	char* randomString = (char*)malloc((length + 1) * sizeof(char));

	for (int i = 0; i < length; ++i) {
		int index = rand() % (sizeof(charset) - 1);
		randomString[i] = charset[index];
	}

	randomString[length] = '\0';
	return randomString;
}


void fill_storage(storage_t* storage) {
	char* str;
    for (int i = 0; i < storage->max_count; i++) {
		str = createString();
        storage_add(storage, str);
    }
    printf("fill_storage: storage filled\n");
}

// int incr - flag for increase or decrease
// can be INCREASE, DEGREASE or EQUAL
int find_pair_count(storage_t* storage, int incr) {
    int pair_count = 0;
    snode_t* cur_node = storage->first;
    snode_t* next_node;
	
    while ((cur_node != NULL) && (cur_node->next != NULL)) {
        //printf("find_pair_count: cur_node = %s\n", cur_node->val);
        //printf("find_pair_count: try to take next_node\n");
        if (cur_node->next != NULL) {
            next_node = cur_node->next;
            //printf("find_pair_count: next_node = %s\n", next_node->val);
        }
        
		if (incr == INCREASE) {
			if (strlen(cur_node->val) <= strlen(next_node->val)) {
				pair_count++;
			}
		}

		else if (incr == DECREASE) {
			if (strlen(cur_node->val) >= strlen(next_node->val)) {
				pair_count++;
                //printf("swapped += swap(storage, cur_node, cur_node->next);\n");
            
			}
		}
		else {
			if (strlen(cur_node->val) == strlen(next_node->val)) {
				pair_count++;
			}
		}

		int cond = generateRandomBit();
			if (cond == 1) {
				swapped += swap(storage, cur_node, cur_node->next);
				swap_attempts++;
			}
       
        cur_node = cur_node->next;

    }
    return pair_count;
}

void* thread_1(void* args) {
    storage_t* storage = (storage_t*)args;
    printf("thread_1 [%d %d %ld]\n", getpid(), getppid(), pthread_self());

    set_cpu(0);

    int pair_count = 0;
	/*while (1) {
		pair_count = find_pair_count(storage, INCREASE);
		printf("thread_1 pair_count = %d\n", pair_count);
		thread1_iter_count++;
	}*/
    pair_count = find_pair_count(storage, INCREASE);

    return NULL;
}

void* thread_2(void* args) {
	storage_t* storage = (storage_t*)args;
	printf("thread_2 [%d %d %ld]\n", getpid(), getppid(), pthread_self());

	set_cpu(1);

	int pair_count = 0;
	/*while (1) {
		pair_count = find_pair_count(storage, DECREASE);
		printf("thread_2 pair_count = %d\n", pair_count);
		thread2_iter_count++;
	}*/

	pair_count = find_pair_count(storage, DECREASE);
	printf("thread_2 pair_count = %d\n", pair_count);
	thread2_iter_count++;

    return NULL;
}

void* thread_3(void* args) {
	storage_t* storage = (storage_t*)args;
	printf("thread_3 [%d %d %ld]\n", getpid(), getppid(), pthread_self());

	set_cpu(2);

	int pair_count = 0;
	while (1) {
		pair_count = find_pair_count(storage, EQUAL);
		printf("thread_3 pair_count = %d\n", pair_count);
		thread2_iter_count++;
	}
	return NULL;
}

int main() {
    //int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    //printf("numcores %d\n", num_cores);
	pthread_t tid;
    storage_t *s;
	int err;
	initialize_random_generator();

	printf("main [%d %d %ld]\n", getpid(), getppid(), pthread_self());

    s = storage_init(12);
    fill_storage(s);

    //storage_print_stats(s);
    
    print_storage(s);

	sched_yield();

    err = pthread_create(&tid, NULL, thread_2, s);
    if (err) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}

	pthread_join(tid, NULL);

    printf("swapped = %d\n", swapped);
    printf("swap_attempts = %d\n", swap_attempts);

	storage_destroy(s);
    return 0;
}
