#include "lib.h"
#include "types.h"

void think(pid_t);
void eat(pid_t);

int uEntry(void) {
	// For lab4.1
	// Test 'scanf' 
	// int dec = 0;
	// int hex = 0;
	// char str[6];
	// char cha = 0;
	// int ret = 0;
	// while(1){
	// 	printf("Input:\" Test %%c Test %%6s %%d %%x\"\n");
	// 	ret = scanf(" Test %c Test %6s %d %x", &cha, str, &dec, &hex);
	// 	printf("Ret: %d; %c, %s, %d, %x.\n", ret, cha, str, dec, hex);
	// 	if (ret == 4) {
	// 		break;
	// 	}
	// }
	
	// For lab4.2
	// Test 'Semaphore'
	// int i = 4;
	// int ret;

	// sem_t sem;
	// printf("Father Process: Semaphore Initializing.\n");
	// ret = sem_init(&sem, 2);
	// if (ret == -1) {
	// 	printf("Father Process: Semaphore Initializing Failed.\n");
	// 	exit();
	// }

	// ret = fork();
	// if (ret == 0) {
	// 	while( i != 0) {
	// 		i --;
	// 		printf("Child Process: Semaphore Waiting.\n");
	// 		sem_wait(&sem);
	// 		printf("Child Process: In Critical Area.\n");
	// 	}
	// 	printf("Child Process: Semaphore Destroying.\n");
	// 	sem_destroy(&sem);
	// 	exit();
	// }
	// else if (ret != -1) {
	// 	while( i != 0) {
	// 		i --;
	// 		printf("Father Process: Sleeping.\n");
	// 		sleep(128);
	// 		printf("Father Process: Semaphore Posting.\n");
	// 		sem_post(&sem);
	// 	}
	// 	printf("Father Process: Semaphore Destroying.\n");
	// 	sem_destroy(&sem);
	// 	exit();
	// }

	// For lab4.3
	// TODO: You need to design and test the philosopher problem.
	// Producer-Consumer problem and Reader& Writer Problem are optional.
	// Note that you can create your own functions.
	// Requirements are demonstrated in the guide.

	// philosopher problem
	// sem_t forks[5];
	// for(int i = 0; i < 5; i++) {
	// 	sem_init(&forks[i], 1);
	// }
	// for(int i = 0; i < 5; i++) {
	// 	if(fork() == 0) {
	// 		volatile pid_t id = getpid();
	// 		// printf("%d", id);
	// 		think(id);
	// 		sleep(128);
	// 		if(id %2 == 0) {
	// 			sem_wait(&forks[id]);
	// 			sem_wait(&forks[(id+1)%5]);
	// 		}else {
	// 			sem_wait(&forks[(id+1)%5]);
	// 			sem_wait(&forks[id]);
	// 		}
	// 		eat(id);
	// 		sleep(128);
	// 		sem_post(&forks[id]);
	// 		sem_post(&forks[(id+1)%5]);
	// 		sleep(128);
	// 		break;
	// 	}
	// }

	//  Producer-Consumer problem
	// int buffer_size = 3;
	// sem_t full;
	// sem_init(&full, 0);
	// sem_t empty;
	// sem_init(&empty, buffer_size);
	// sem_t mutex;
	// sem_init(&mutex, 1);

	// for(int i = 0; i < 5; i++) {
	// 	if(fork() == 0) {
	// 		if(i == 4) { // Consumer
	// 			while(1) {
	// 				sem_wait(&full);
	// 				sleep(128);
	// 				sem_wait(&mutex);
	// 				sleep(128);
	// 				printf("Consumer consumes...\n");
	// 				sleep(128);
	// 				sem_post(&mutex);
	// 				sleep(128);
	// 				sem_post(&empty);
	// 			}
	// 		}else {
	// 			while(1) {
	// 				pid_t id = getpid();
	// 				sem_wait(&empty);
	// 				sleep(128);
	// 				sem_wait(&mutex);
	// 				sleep(128);
	// 				printf("Producer %d produces...\n", id);
	// 				sleep(128);
	// 				sem_post(&mutex);
	// 				sleep(128);
	// 				sem_post(&full);
	// 			}
	// 		}
	// 	}
	// }

	//  Reader& Writer Problem
	#define ADD 1
	#define  SUB 0
	var_init(0);
	sem_t WriteMutex;
	sem_init(&WriteMutex, 1);
	sem_t CountMutex;
	sem_init(&CountMutex, 1);
	for(int i = 0; i < 6; i++) {
		if(fork() == 0) {
			if(i > 2) { // Reader
				while(1) {
					pid_t id = getpid();
					sem_wait(&CountMutex);
					sleep(128);
					if(var_change(ADD, 0) == 0) {
						sem_wait(&WriteMutex);
					}
					var_change(ADD, 1);
					sem_post(&CountMutex);
					printf("Reader %d: read, total %d reader\n", id-4, var_change(ADD, 0));
					sleep(128);
					sem_wait(&CountMutex);
					var_change(SUB, 1);
					if(var_change(ADD, 0) == 0) {
						sem_post(&WriteMutex);
					}
					sleep(128);
					sem_post(&CountMutex);
				}
			}else {
				while(1) {
					pid_t id = getpid();
					sem_wait(&WriteMutex);
					sleep(128);
					printf("Writer %d: write\n", id-1);
					sem_post(&WriteMutex);
					sleep(128);
				}
			}
		}
	}

	while(1) ;
	return 0;
}

void think(pid_t id) {
	printf("%d Thinking...\n", id);
}
void eat(pid_t id) {
	printf("%d Eating...\n", id);
}
