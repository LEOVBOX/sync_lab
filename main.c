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
#include <signal.h>

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

int debug_mode = 0;
int sync_option = 0;

void sig_handler(int signum) {
	if (signum == SIGSEGV) {
		printf("SIGSEGV\n");
	}
	printf("\n");
	printf("thread1_iter_count = %d\n", thread1_iter_count);
	printf("thread2_iter_count = %d\n", thread2_iter_count);
	printf("thread3_iter_count = %d\n", thread3_iter_count);
	printf("swapped = %d\n", swapped);
	printf("swap_attempts = %d\n", swap_attempts);
	exit(0);
}



void initialize_random_generator() {
	srand(time(NULL));
}

int generate_random_bit() {
    int result = rand() % 2;
	//printf("generateRandomBit: result = %d\n", result);
	return result;
}

int count_digits(int number) {
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
	char* result = (char*)malloc(count_digits(number) * sizeof(char));

	sprintf(result, "%d", number);

	return result;
}

char* create_string() {
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
		str = create_string();
        storage_add(storage, str);
    }
}

// int incr - flag for increase or decrease
// can be INCREASE, DEGREASE or EQUAL
int find_pair_count(storage_t* storage, int incr) {
    int pair_count = 0;
    snode_t* cur_node = storage->head;

	int flag = 0;

    while (cur_node->next != NULL && cur_node->next->next != NULL) {
		if (sync_option == SPINLOCK) {
			pthread_spin_lock(&cur_node->spinlock);
			pthread_spin_lock(&cur_node->next->spinlock);
			pthread_spin_lock(&cur_node->next->next->spinlock);	
		}

		else {
			pthread_mutex_lock(&cur_node->mutex);
			pthread_mutex_lock(&cur_node->next->mutex);
			pthread_mutex_lock(&cur_node->next->next->mutex);
		}

        //printf("find_pair_count: cur_node = %s\n", cur_node->val);
        //printf("find_pair_count: try to take next_node\n");
		if (debug_mode) {
			printf("\n\n%ld\ncur_node = %s\ncur_node->next = %s\ncur_node->next->next = %s\n", 
			pthread_self(), cur_node->val, cur_node->next->val, cur_node->next->next->val);
		}
		if (incr == INCREASE) {
			if (strlen(cur_node->next->val) <= strlen(cur_node->next->next->val)) {
				pair_count++;
			}
		}

		else if (incr == DECREASE) {
			if (strlen(cur_node->next->val) >= strlen(cur_node->next->next->val)) {
				pair_count++;
                //printf("swapped += swap(storage, cur_node, cur_node->next);\n");
            
			}
		}
		else {
			if (strlen(cur_node->next->val) == strlen(cur_node->next->next->val)) {
				pair_count++;
			}
		}

		int cond = generate_random_bit();
			if (cond == 1) {
				swapped += swap(storage, cur_node, cur_node->next, cur_node->next->next);
				swap_attempts++;
			}

		if (sync_option == SPINLOCK) {
			pthread_spin_unlock(&cur_node->spinlock);
			pthread_spin_unlock(&cur_node->next->spinlock);
			pthread_spin_unlock(&cur_node->next->next->spinlock);	
		}

		else {
			pthread_mutex_unlock(&cur_node->mutex);
			pthread_mutex_unlock(&cur_node->next->mutex);
			pthread_mutex_unlock(&cur_node->next->next->mutex);
		}
		
        cur_node = cur_node->next;
    }

	
    return pair_count;
}

void* thread_1(void* args) {
    storage_t* storage = (storage_t*)args;
    printf("thread_1 [%d %d %ld]\n", getpid(), getppid(), pthread_self());

    set_cpu(1);

    int pair_count = 0;
	if (debug_mode) {
		pair_count = find_pair_count(storage, INCREASE);
		printf("thread_1 pair_count = %d\n", pair_count);
		thread1_iter_count++;
		print_storage(storage);
	}
	else {
		int i = 0;
		while (1) {
			pair_count = find_pair_count(storage, INCREASE);
			if (i == 0) {
				printf("thread_1: pair_count = %d\n", pair_count);
				i++;
			}
			thread1_iter_count++;
		}
	}
	
    return NULL;
}

void* thread_2(void* args) {
	storage_t* storage = (storage_t*)args;
	printf("thread_2 [%d %d %ld]\n", getpid(), getppid(), pthread_self());

	set_cpu(2);

	int pair_count = 0;
	if (debug_mode) {
		pair_count = find_pair_count(storage, DECREASE);
		printf("thread_2 pair_count = %d\n", pair_count);
		thread2_iter_count++;
	}

	else {
		int i = 0;
		while (1) {
			pair_count = find_pair_count(storage, DECREASE);
			if (i == 0) {
				printf("thread_2: pair_count = %d\n", pair_count);
				i++;
			}
			thread2_iter_count++;
		}
	}
	

	
    return NULL;
}

void* thread_3(void* args) {
	storage_t* storage = (storage_t*)args;
	printf("thread_3 [%d %d %ld]\n", getpid(), getppid(), pthread_self());

	set_cpu(3);

	int pair_count = 0;

	if (debug_mode) {
		pair_count = find_pair_count(storage, EQUAL);
		printf("thread_3 pair_count = %d\n", pair_count);
		thread3_iter_count++;
		print_storage(storage);
	}

	else {
		int i = 0;
		while (1) {
			pair_count = find_pair_count(storage, EQUAL);
			pair_count = find_pair_count(storage, DECREASE);
			if (i == 0) {
				printf("thread_3: pair_count = %d\n", pair_count);
				i++;
			}
			thread2_iter_count++;
			thread3_iter_count++;
		}
	}
	
	return NULL;
}

int argv_parser(char* arg) {
	if (strcmp(arg, "s") == 0) {
		return SPINLOCK;
	}
	else if (strcmp(arg, "m") == 0) {
		return MUTEX;
	}
	return 0;
}

int main(int argc, char* argv[]) {
    //int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    //printf("numcores %d\n", num_cores);
	if (argc < 2) {
		printf("few arguments\n");
		exit(1);
	}

	if ((argc > 3) && (strcmp(argv[2], "debug") == 0)) {
		printf("debug_mode\n");
		debug_mode = 1;
	}

	if (sync_option = argv_parser(argv[1]) == 0) {
		printf("invalid sync_option\n");
		return 1;
	}
	
	printf("sync_option = %d\n", sync_option);



	signal(SIGINT, sig_handler);
	signal(SIGSEGV, sig_handler);

	pthread_t tid;
    storage_t *s;

	int err;
	initialize_random_generator();

	printf("main [%d %d %ld]\n", getpid(), getppid(), pthread_self());

    s = storage_init(100, debug_mode);
	if (debug_mode) {
		s->debug_mode = 1;
	}

    fill_storage(s);

    //storage_print_stats(s);


    err = pthread_create(&tid, NULL, thread_1, s);
    if (err) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}

	err = pthread_create(&tid, NULL, thread_2, s);
	if (err) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}

	err = pthread_create(&tid, NULL, thread_3, s);
	if (err) {
		printf("main: pthread_create() failed: %s\n", strerror(err));
		return -1;
	}

	pthread_exit(NULL);
	//pthread_join(tid, NULL);

	storage_destroy(s);
    return 0;
}
