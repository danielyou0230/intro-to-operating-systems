#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <sched.h>

#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 3
#define REQUEST 0
#define RELEASE 1

typedef struct 
{
	int customerNum;
	int request[NUMBER_OF_RESOURCES];
} client;

// debugging for conditional randomness
int almost = 0;
//
pthread_mutex_t mutex;
void *threadRunner(void *person);
void show_current_state(void);
void show_operation_info(int operation, int customerNum, char info[], int code, int resource[]);
int requestResources(int customerNum, int request[]);
int releaseResources(int customerNum, int release[]);

// The available amount of each resource
int available[NUMBER_OF_RESOURCES]; 
// The maximum demand of each customer
int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES] = {};
// The amount currently allocated to each customer
int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES] = {};
// The remaining need of each customer
int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
// The finishing count of the system,
// if a cutomer acquire all needs, finishCount++
int finishCount = 0;
//
bool finish[NUMBER_OF_CUSTOMERS] = {false};

int main(int argc, char const *argv[])
{
	int i, j;
	pthread_t tid[NUMBER_OF_CUSTOMERS];
	client person[NUMBER_OF_CUSTOMERS];
	// Initialising
	srand((unsigned)time(NULL));
	printf("Initialising tables...\n\n");
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i) {
		available[i] = atoi(argv[i + 1]);
		for (j = 0; j < NUMBER_OF_CUSTOMERS; ++j) {
			while (maximum[j][i] == 0)
				maximum[j][i] = rand() % (available[i] + 1);
			need[j][i] = maximum[j][i] - allocation[j][i];
		}
	}
	show_current_state();
	
	// Multithreading
	for (i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
		// printf("Creating thread for client #%d\n", i);
		person[i].customerNum = i;
		//
		pthread_create(&tid[i], NULL, threadRunner, &person[i]);
	}
	//
	for (i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
	{
		pthread_join(tid[i], NULL);
	}
	
	return 0;
}

void show_current_state(void) {
	int i, j;
	printf("current state\n\n");
	printf("available\nresources      ");
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i) {
		printf("%*d ", 2, available[i]);
	}
	printf("\n");
	// printf("______________________________________________________\n");
	printf("             |  maximum  | allocation |   need    |  finish  |\n");
	//printf("______________________________________________________\n");
	for (i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
		printf("  cutomer %d  | ", i);
		for (j = 0; j < NUMBER_OF_RESOURCES; ++j)
			printf("%*d ", 2, maximum[i][j]); 
		printf(" |  ");
		for (j = 0; j < NUMBER_OF_RESOURCES; ++j)
			printf("%*d ", 2, allocation[i][j]); 
		printf(" | ");
		for (j = 0; j < NUMBER_OF_RESOURCES; ++j)
			printf("%*d ", 2, need[i][j]); 
		printf(" |  ");
		printf("%*s  ", 5, finish[i] ? "true" : "false");
		printf(" |\n");
	}
	printf("================================================================\n\n");
}

void show_operation_info(int operation, int customerNum, char info[], int code, int resource[]) {
	int i, j;
	char type[10];
	// 
	if (operation == REQUEST)
		strcpy(type, "Request");
	else if (operation == RELEASE)
		strcpy(type, "Release");
	else 
		strcpy(type, "Unknown");
	//
	printf("%s  ", type);
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i) 
		printf("%*d ", 2, resource[i]);
	printf(" (from customer %d)\n", customerNum);
	printf("%s Code %d: %s\n", type, code, info);
}

void *threadRunner(void *person) {
	int i, j = 0;
	int code;
	client *p = (client *) person;
	int cond_count = 0;
	// 
	while (true) {
	// while (j < 7) {
		// Generate the amount of resource for request 
		for (i = 0; i < NUMBER_OF_RESOURCES; ++i) {
			p->request[i] = rand() % (maximum[p->customerNum][i] + 1);
		}
		// Prevent race conditions
		pthread_mutex_lock(&mutex);
		// Access share data
		/* 
		// Early Stopping
		almost = 0;
		for (i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
			if (finish[i] == true)
				++almost;
		} 
		if (almost == 3) {
			for (i = 0; i < NUMBER_OF_RESOURCES; ++i)
				p->request[i] = need[p->customerNum][i];
			// getchar();
		}
		*/
		//
		code = requestResources(p->customerNum, p->request);
		//
		pthread_mutex_unlock(&mutex);
		// printf("$$$ yield cpu from customer %d (request)\n", p->customerNum);
		sched_yield();
		//
		++j;
		if (finish[p->customerNum] == true)
			break;
		// release resource randomly
		pthread_mutex_lock(&mutex);
		//
		for (i = 0; i < NUMBER_OF_RESOURCES; ++i) {
			p->request[i] = rand() % (maximum[p->customerNum][i] + 1);
		}
		releaseResources(p->customerNum, p->request);
		//
		pthread_mutex_unlock(&mutex);
		// printf("$$$ yield cpu from customer %d (release)\n", p->customerNum);
		sched_yield();
	}
	pthread_exit(0);
}

int requestResources(int customerNum, int request[]) {
	/* 
	return code
	+3: succeeds & all customers finish
	+2: succeeds & current customer finishes
	+1: succeeds
	 0: no request
	-1: fails since request exceeds need
	-2: fails since request exceeds available
	-3: fails since the state is unsafe
	*/
	int i, j, code;
	int count = 0;
	int unsafe = 0;
	int safe = 0;
	bool valid = true;
	char info[100];
	int work[NUMBER_OF_RESOURCES];
	bool tmp_finish[NUMBER_OF_CUSTOMERS];
	//
	printf("[REQUEST] Received request from customer %d\n", customerNum);
	// 1. check request exceed need
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i) {
		if (request[i] > need[customerNum][i]) {
			code = -1;
			sprintf(info, "customer %d's request fails since request exceeds need\n", customerNum);
			goto END_REQUEST;
		}
	}
	// 2. check request exceed available
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i) {
		if (request[i] > available[i]) {
			code = -2;
			sprintf(info, "customer %d's request fails since request exceeds available\n", customerNum);
			goto END_REQUEST;
		}
	}
	// 3. pretend to allocate resources
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i) {
		available[i] -= request[i];
		allocation[customerNum][i] += request[i];
		// update need
		// need[customerNum][i] -= request[i];
		need[customerNum][i] = maximum[customerNum][i] - allocation[customerNum][i];
	}
	// printf("Allocated resources for customer %d\n", customerNum);
	// 4. check finishAll, finish, no request with precedence: finishAll > finish > no request
	// release allocated resources if finishAll or finish occur
	// 
	// check if finish
	count = 0;
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i) {
		if (need[customerNum][i] == 0)
			++count;
	}
	// 
	if (count == NUMBER_OF_RESOURCES)
		finish[customerNum] = true;
	// (1) check finishAll
	count = 0;
	for (i = 0; i < NUMBER_OF_CUSTOMERS; ++i) {
		if (finish[i])
			++count;
	}
	//     all finish
	if (count == NUMBER_OF_CUSTOMERS) {
		code = 3;
		sprintf(info, "customer %d's request succeeds & all customers finish\n", customerNum);
		goto END_REQUEST;
	}
	//
	// (2) check finish (current): if need[i] == 0
	//     current finish
	// if (count == NUMBER_OF_RESOURCES) {
	if (finish[customerNum] == true) {
		// finish[customerNum] = true;
		code = 2;
		sprintf(info, "customer %d's request succeeds & current customer finishes\n", customerNum);
		goto END_REQUEST;
	}
	//
	// (3) no request: if request[i] == 0
	count = 0;
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i) {
		if (request[i] == 0)
			++count;
	}
	//     no request
	if (count == NUMBER_OF_RESOURCES) {
		code = 0;
		sprintf(info, "customer %d doesn't request\n", customerNum);
		goto END_REQUEST;
	}
	//
	// 5. check safety, if not safe, restore the allocated resources in step 3.
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i)
		work[i] = available[i];
	for (i = 0; i < NUMBER_OF_CUSTOMERS; ++i)
		tmp_finish[i] = finish[i];
	//
	printf("pre-allocated:\n");
	show_current_state();
	// check safety
	i = 0;
	while (safe < NUMBER_OF_CUSTOMERS && unsafe < NUMBER_OF_CUSTOMERS) {
		if (tmp_finish[i] == false) {
			// printf("&&& checking for customer %d\n", i);
			// elementwisely check need <= work 
			for (j = 0; j < NUMBER_OF_RESOURCES; ++j) {
				if (need[i][j] > work[j]) {
					valid = false;
					goto NEXT_CLIENT;
				}
				else // check next need <= work
					continue;
			}
			// unfinished but is valid to complete
			if (valid) {
				// printf("@@@ customer %d is valid\n", i);
				for (j = 0; j < NUMBER_OF_RESOURCES; ++j) {
					work[j] += allocation[i][j];
				}
				tmp_finish[i] = true;
				// reset "unsafe" indicator if one of the client can be completed
				unsafe = 0;
				// continue to check next client
			}
			// unfinished and resource unavailable
			// else
			goto NEXT_CLIENT;
		}
		NEXT_CLIENT:
		// record if the client has no sufficient resource
		if (!valid) {
			unsafe += 1;
			// printf("@@@ customer %d is invalid (unsafe = %d)\n", i, unsafe);
		}
		// reset "valid" indicator
		valid = true;
		// circular search
		i = (i + 1) % NUMBER_OF_CUSTOMERS;
		// check if safe to go (safe == NUMBER_OF_CUSTOMERS)
		safe = 0;
		for (j = 0; j < NUMBER_OF_CUSTOMERS; ++j) {
			if (tmp_finish[j] == true)
				++safe;
		}
	}
	//
	if (unsafe == NUMBER_OF_CUSTOMERS) {
		// restore resources just allocated
		for (i = 0; i < NUMBER_OF_RESOURCES; ++i) {
			available[i] += request[i];
			allocation[customerNum][i] -= request[i];
			// update need
			need[customerNum][i] = maximum[customerNum][i] - allocation[customerNum][i];
		}
		//
		code = -3;
		sprintf(info, "customer %d's request fails since the state is unsafe\n", customerNum);
		goto END_REQUEST;
	}
	else if (safe == NUMBER_OF_CUSTOMERS) {
		code = 1;
		sprintf(info, "customer %d's request succeeds\n", customerNum);
		goto END_REQUEST;
	}
	else { // exception 
		sprintf(info, "*** request exception ***\n");
		return -4;
	}
	//
	// return request code
	END_REQUEST: 
	printf("\n");
	show_operation_info(REQUEST, customerNum, info, code, request);
	show_current_state();
	// release all allocated resource if finished
	if (finish[customerNum] == true) {
		for (i = 0; i < NUMBER_OF_RESOURCES; ++i) {
			request[i] = allocation[customerNum][i];
		}
		releaseResources(customerNum, request);
	}
	return code;
}

int releaseResources(int customerNum, int release[]) {
	/*
	return code 
	+1: succeeds 
	 0: no release
	-1: fails since release exceeds allocation
	*/
	int i, code;
	int count = 0;
	char info[100];
	//
	if (finish[customerNum] == true)
		printf("[RELEASE] Release all reources allocated for customer %d\n", customerNum);
	else
		printf("[RELEASE] Received release from customer %d\n", customerNum);
	// 1. check release exceed allocation
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i) {
		if (release[i] > allocation[customerNum][i]) {
			code = -1;
			sprintf(info, "customer %d's release fails since release exceeds allocation\n", customerNum);
			goto END_RELEASE;
		}
	}
	// 2. check no release
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i) {
		if (release[i] == 0) {
			++count;
			printf("%d\n", release[i]);
		}
	}
	if (count == NUMBER_OF_RESOURCES) {
		code = 0;
		sprintf(info, "customer %d doesn't release any resource\n", customerNum);
		goto END_RELEASE;
	}
	// 3. release
	for (i = 0; i < NUMBER_OF_RESOURCES; ++i) {
		if (finish[customerNum] == true) { // release all (finished)
			available[i] += allocation[customerNum][i];
			allocation[customerNum][i] = 0;
			// update need
			need[customerNum][i] = 0;
		}
		else { // normal release
			available[i] += release[i];
			allocation[customerNum][i] -= release[i];
			// update need
			need[customerNum][i] = maximum[customerNum][i] - allocation[customerNum][i];
		}
	}
	code = 1;
	sprintf(info, "customer %d's release succeeds\n", customerNum);
	goto END_RELEASE;
	//
	// return release code
	END_RELEASE:
	show_operation_info(RELEASE, customerNum, info, code, release);
	show_current_state();
	return code; 
}

