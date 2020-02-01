#include "bbuff.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <semaphore.h>

/*
void bbuff_init(void);
void bbuff_blocking_insert(void* item);
void* bbuff_blocking_extract(void);
_Bool bbuff_is_empty(void);
*/

//make the buffer array declared in the .c file)
// of type void* such as: void* da_data[DA_SIZE];

void* bufferData[BUFFER_SIZE];


//Note: Page 219, 5.7.1 The Bounded Buffer Problem
//		Initialize it in void bbuff_init()
int n; // item counter
sem_t mutex; // provides mutual exclusion for access to the buffer pool
sem_t empty; // counts the number of empty buffers
sem_t full; // counts the number of full buffers


void bbuff_init(void) {
	n = 0; // 0 items in buffer
	//int sem_init(sem_t *sem, int pshared, unsigned int value);
	sem_init(&mutex, 0, 1); // mutex is initialized to 1
	sem_init(&empty, 0, BUFFER_SIZE); // empty is set to 10 because it has 10 free spaces
	sem_init(&full, 0, 0); // full is initialized to 0 because buffer is not full
	
}


void bbuff_blocking_insert(void* item) {
	//Page 219, 5.7.1 The Bounded Buffer Problem
	sem_wait(&empty);
	sem_wait(&mutex);

	// Add next_produced to the buffer 
	bufferData[n] = item;
	n++;

	sem_post(&mutex);
	sem_post(&full);
}

void* bbuff_blocking_extract(void) {
	//Page 220, 5.7.1 The Bounded Buffer Problem
	sem_wait(&full);
	sem_wait(&mutex);

	//decrement n to account for array element numbering
	n--;
	void* item = bufferData[n];
	bufferData[n] = NULL;

	sem_post(&mutex);
	sem_post(&empty);

	return item;
}

_Bool bbuff_is_empty(void) {
	if(n == 0) {
		return true;
	}else {
		return false;
	}
}
