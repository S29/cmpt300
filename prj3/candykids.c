#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "bbuff.h"
#include "stats.h"
#include <math.h>
#include <stdbool.h>

/*
	rand() % 4 gives numbers 0-3
	rand() % 2 gives numbers 0-1
*/



typedef struct  {
    int factory_number;
    double time_stamp_in_ms;
} candy_t;

_Bool stop_thread = false; // Global Variables are shared amongst threads

double current_time_in_ms(void)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now.tv_sec * 1000.0 + now.tv_nsec/1000000.0;
}

void* factoryThread(void *num) {
	//1. Loop until main() signals to exit (see below)
	int fNum = *((int *) num);
	while(!stop_thread) { 
		//2. Pick a number of seconds which it will (later) wait.
		//   Number randomly selected between 0 and 3 inclusive.
		int randNum = rand() % 4;
		//3. Print a message such as: "\tFactory 0 ships candy & waits 2s"
		printf("\tFactory %d ships candy & waits %ds\n", fNum, randNum);

		//4. Dynamically allocate a new candy item and populate its fields.

		candy_t *candy = malloc(sizeof(candy_t)); //* sizeof(*candy));
		candy->factory_number = fNum;
		candy->time_stamp_in_ms = current_time_in_ms();

		//5. Add the candy item to the bounded buffer.
		bbuff_blocking_insert(candy);
		stats_record_produced(fNum);
		//6. Sleep for number of seconds identified in #1.
		
		sleep(randNum);
		

	}
	//7.When the thread finishes, print the message such as the following
	// (for thread 0): "Candy-factory 0 done"
	printf("Candy-factory %d done\n", fNum);
	pthread_exit(NULL);
}

void* kidThread(void* nothing) {
	//1. Loop forever
	while(true) {
		//2. Extract a candy item from the bounded buffer. 

		candy_t* candyBuff = bbuff_blocking_extract();
		
		//3. Process the item. Initially you may just want to printf() it to the screen;
		// in the next section, you must add a statistics module that will track what 
		// candies have been eaten.
		if (candyBuff != NULL) {
			stats_record_consumed(candyBuff->factory_number, current_time_in_ms() - candyBuff->time_stamp_in_ms);
            //printf("test: %f\n", current_time_in_ms() - candyBuff->time_stamp_in_ms);
		}
        
		//4. Sleep for either 0 or 1 seconds (randomly selected). The kid threads are
		//   canceled from main() using pthread_cancel(). When this occurs, it is 
		//   likely that the kid thread will be waiting on the semaphore in the 
		//   bounded buffer. This should not cause problems.
		int randNum = rand() % 2;
        free(candyBuff);
		sleep(randNum);
	}
	pthread_exit(NULL);
}


int main(int argc, char* argv[]) {
    // 1.  Extract arguments
	int factories, kids, seconds;

	factories = atoi(argv[1]);
	kids = atoi(argv[2]);
	seconds = atoi(argv[3]);

	if(factories <= 0 || kids <= 0 || seconds <= 0) {
		printf("Error: All arguments must be positive.\n");
		exit(0);
	}

    // 2.  Initialize modules
	bbuff_init();
	stats_init(factories);

    // 3.  Launch candy-factory threads

    // - Hint: Store the thread IDs in an array 
    //because you'll need to join on them later.

	pthread_t factoryThreads[factories];

	int factoryNum[factories]; // using this because of the hint
	//Spawn the requested number of candy-factory threads.
	//To each thread, pass it its factory number: 0 to (number of factories - 1).
	for(int i = 0; i < factories; i++) {
		factoryNum[i] = i;
        pthread_t id;
		if(pthread_create(&id, NULL, factoryThread, &factoryNum[i])) {
            printf("Factory Thread Failed.\n");
            exit(1);
        }
        factoryThreads[i] = id;
	}

    // 4.  Launch kid threads

    // Spawn the requested number of kid threads.

    pthread_t kidThreads[kids];

    for(int i = 0; i < kids; i++) {
        pthread_t id;
    	if(pthread_create(&id, NULL, kidThread, NULL)) {
            printf("Kid Thread Failed.\n");
            exit(1);
        }
        kidThreads[i] = id;
    }


    // 5.  Wait for requested time

    //In a loop, call sleep(1). 
    //Loop as many times as the "# Seconds" command line argument.
    //print the number of seconds running each time
    for(int i = 0; i < seconds; i++) {
    	sleep(1);
    	printf("Time %ds\n", i);
    }

    // 6.  Stop candy-factory threads

    stop_thread = true;
    printf("Stopping candy factories...\n");
    for(int i = 0; i < factories; i++) {
    	pthread_join(factoryThreads[i], NULL);
    }

    // 7.  Wait until no more candy
    if(!bbuff_is_empty()) {
    	while(!bbuff_is_empty()) {
    		printf("Waiting for all candy to be consumed.\n");
    		sleep(1);
    	}
    }

    // 8.  Stop kid threads
    printf("Stopping kids.\n");
    for(int i = 0; i < kids; i++) {
        //After a canceled thread has terminated, a join with that thread using
        //pthread_join(3) obtains PTHREAD_CANCELED as the thread's exit status.
        // (Joining with a thread is the only way to know that cancellation has completed.)
    	pthread_cancel(kidThreads[i]);
    	pthread_join(kidThreads[i], NULL);
    }


    // 9.  Print statistics
    stats_display();

    // 10. Cleanup any allocated memory
    stats_cleanup();

    return 0;
}