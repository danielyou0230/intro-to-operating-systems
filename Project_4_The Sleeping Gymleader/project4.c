#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define IDLE   0
#define BATTLE 1
#define SLEEP  2
/* Define struct to pass parameters*/
typedef struct 
{
	char name[10];
	int elapse;
	int combo_left;
} character;

struct 
{
	pthread_mutex_t chair_mutex;
	pthread_mutex_t battle_mutex;
	pthread_cond_t battling;
	pthread_cond_t next_chair;
	int current_state;
	int complete;
	char* chair[2];
	char* chair_next;
} share_mem;

char seq[6][10];
int seq_i;
//
void *begin_challenge(void* challenger);
void *gymleader(void*);
//
int main(int argc, char const *argv[])
{
	// thread / thread attr / mutex
	pthread_t tid[4];
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_mutex_init(&share_mem.chair_mutex, NULL);
	pthread_mutex_init(&share_mem.battle_mutex, NULL);
	
	// initialize share variables
	seq_i = 0;
	share_mem.current_state = IDLE;
	share_mem.chair[0] = NULL;
	share_mem.chair[1] = NULL;
	share_mem.chair_next = NULL;
	share_mem.complete = 0;
	
	// Initialize characters
	character challenger[3];
	strcpy(challenger[0].name, "Ash");
	challenger[0].elapse = 2;
	challenger[0].combo_left = 3;
	strcpy(challenger[1].name, "Misty");
	challenger[1].elapse = 3;
	challenger[1].combo_left = 2;
	strcpy(challenger[2].name, "Brock");
	challenger[2].elapse = 5;
	challenger[2].combo_left = 1;
	
	// Creating threads
	printf("Creating threads\n");
	pthread_create(&tid[0], &attr, begin_challenge, &challenger[0]);
	pthread_create(&tid[1], &attr, begin_challenge, &challenger[1]);
	pthread_create(&tid[2], &attr, begin_challenge, &challenger[2]);
	
	// Create and join the gymleader thread
	pthread_create(&tid[3], NULL, gymleader, NULL);
	pthread_join(tid[3], NULL);
	
	return 0;
}

void *begin_challenge(void* param) {
	//
	int on_chair;
	character *challenger = (character *) param;
	while (challenger->combo_left) {
		on_chair = 0; // no available chairs
		pthread_mutex_lock(&share_mem.chair_mutex);
		// Acquire Chair or Go training
		while (on_chair <= 0) {
			// check chairs empty
			if (share_mem.chair[0] == NULL && share_mem.chair[1] == NULL) {
				share_mem.chair[0] = &challenger->name;
				printf("[CHAIR]  Challenger %s acquired chair #1\n", share_mem.chair[0]);
				on_chair = 1;
				pthread_mutex_unlock(&share_mem.chair_mutex);
			}
			// Chair #2 is available:
			else if (share_mem.chair[1] == NULL) {
				share_mem.chair[1] = &challenger->name;
				printf("[CHAIR]  Challenger %s acquired chair #2\n", share_mem.chair[1]);
				on_chair = 2;
				pthread_mutex_unlock(&share_mem.chair_mutex);
			}
			// Go for training
			else {
				printf("[TRAIN]  No chairs available. Trainer %s is going out for training.\n", challenger->name);
				sleep(challenger->elapse);
				pthread_cond_wait(&share_mem.next_chair, &share_mem.chair_mutex);
			}
		}
		// Check if gymleader is available for battle
		pthread_mutex_lock(&share_mem.battle_mutex);
		while (share_mem.chair[0] != challenger->name) {
			printf("[BLOCK]  Trainer %s is waiting on the chair.\n", challenger->name);
			pthread_cond_wait(&share_mem.battling, &share_mem.battle_mutex);
			--on_chair;
			printf("[UNBLK]  Trainer %s is ready to battle. (waked up)\n", challenger->name);
		}
		// Battle available, begin
		if (share_mem.current_state == SLEEP) { // wakeup
			printf("[BATTLE] The Gym Leader is waken up by Trainer %s.\n", challenger->name);
		}
		share_mem.current_state = BATTLE;
		pthread_mutex_unlock(&share_mem.battle_mutex);
		//
		// Battling
		printf("[BATTLE] The Gym Leader is battling with Trainer %s now.\n", challenger->name);
		sleep(challenger->elapse);
		//
		// Battle is over, acquire lock
		 pthread_mutex_lock(&share_mem.battle_mutex);
		share_mem.current_state = IDLE;
		printf("[BATTLE] The battle with Trainer %s is over.\n", challenger->name);
		// Release lock
		pthread_mutex_unlock(&share_mem.battle_mutex);
		// INFO
		--challenger->combo_left;
		printf("[INFO]   Trainer %s has %d time(s) left to finish the game.\n\n", challenger->name, challenger->combo_left);
		// Acquire lock
		pthread_mutex_lock(&share_mem.chair_mutex);
		// Left chair
		if (share_mem.chair[1] != NULL){
			printf("[LEFT]   Trainer %s left the chair, next challenger is %s\n", share_mem.chair[0], share_mem.chair[1]);
			share_mem.chair[0] = share_mem.chair[1];
			share_mem.chair[1] = NULL;
		}
		else if (share_mem.chair[0] == NULL && share_mem.chair[1] == NULL) {
			printf("[LEFT]   Trainer %s left the chair, no one's waiting.\n", share_mem.chair[0]);
			share_mem.chair[0] = NULL;
			printf("[SLEEP]  The Gym Leader goes to sleep now.\n");
			share_mem.current_state = SLEEP;
		}
		else { // share_mem.chair[0] has finished battle, and no one is waiting
			// Clear the chairs
			share_mem.chair[0] = NULL;
			share_mem.chair[1] = NULL;
			printf("[CHAIR]  Chairs are cleared.\n");
		}
		//
		// Broadcast signals
		pthread_cond_broadcast(&share_mem.battling);
		pthread_cond_broadcast(&share_mem.next_chair);
		pthread_mutex_unlock(&share_mem.chair_mutex);
		//
		// Record battle sequence
		strcpy(seq[seq_i], challenger->name);
		++seq_i;
	}
	printf("[COMP]   Trainer %s has finished the game.\n", challenger->name);
	// Signal complete
	++share_mem.complete;
	return NULL;
}

void *gymleader(void* param) {
	int i;
	while (share_mem.complete < 3) {
		// wait to collect complete signal
		 sleep(0.05);
	}
	// Print Battle sequence
	printf("\nSequence: ");
	for (i = 0; i < 6; ++i){
		printf("%s", seq[i]);
		if (i < 5)
			printf(" -> ");
	}
	printf("\n");
	//
	pthread_exit(0);
}