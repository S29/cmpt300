#include "stats.h"
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

/*
array of structs acts as a hashmap
key-> element number = factory number
value-> struct = factory information



*/

sem_t mutex;

typedef struct {
	//  Factory#   #Made  #Eaten  Min Delay[ms]  Avg Delay[ms]  Max Delay[ms]
	int made;
	int eaten;
	double min_delay;
	double total_delay;
	double max_delay;
} candy_factory;

candy_factory* candyfactory;
int producers; //need this to print the stats
void stats_init(int num_producers) {
	producers = num_producers;
	candyfactory = malloc(sizeof(candy_factory) * num_producers);

	for(int i = 0; i < num_producers; i++) {
		candyfactory[i].made = 0;
	 	candyfactory[i].eaten = 0;
	 	candyfactory[i].min_delay = 99999999;
	 	candyfactory[i].total_delay = 0;
	 	candyfactory[i].max_delay = 0;
	}

	sem_init(&mutex, 0, 1);
}

void stats_cleanup(void) {
	free(candyfactory);
}

void stats_record_produced(int factory_number) {
	sem_wait(&mutex);
	candyfactory[factory_number].made++;
	sem_post(&mutex);
}

void stats_record_consumed(int factory_number, double delay_in_ms) {
	sem_wait(&mutex);
	candyfactory[factory_number].eaten++;
	candyfactory[factory_number].total_delay += delay_in_ms;
	/*
	if the delay is smaller than the current minimum, then change the current miniumum
	to the new delay value
	similar thing occurs with max
	*/
	if(delay_in_ms < candyfactory[factory_number].min_delay) {
		candyfactory[factory_number].min_delay = delay_in_ms;
	}

	if(delay_in_ms > candyfactory[factory_number].max_delay) {
		candyfactory[factory_number].max_delay = delay_in_ms;
	}
	sem_post(&mutex);
}

void stats_display(void) {
	//Hint: For the title row, use the following idea:
	//printf("%8s%10s%10s\n", "First", "Second", "Third");
	printf("Statistics:\n");
	printf("   %8s%10s%10s%15s%15s%15s\n", "Factory#", "#Made", "#Eaten", "Min Delay[ms]", "Avg Delay[ms]", "Max Delay[ms]");
	
	//Hint: For the data rows:
	//printf("%8d%10.5f%10.5f\n", 1, 2.123456789, 3.14157932523);
	
	for(int i = 0; i < producers; i++) {
		int made = candyfactory[i].made;
		int eaten = candyfactory[i].eaten;
		double minDelay = candyfactory[i].min_delay;
		double avgDelay = candyfactory[i].total_delay;
		if(eaten > 0) { // just in case, so it doesn't ever divide by 0
			avgDelay = avgDelay / eaten;
		}
		
		double maxDelay = candyfactory[i].max_delay;
		if(made != eaten) {
			printf("made != eaten\n");
		}else{
			printf("   %8d%10d%10d%15f%15f%15f\n", i, made, eaten, minDelay, avgDelay, maxDelay);
		}
	}
}
