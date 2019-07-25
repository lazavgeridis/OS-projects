/* monitor.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#include "SharedMem.h"

volatile sig_atomic_t exit_signal = 0;
void sig_handler(int signum) { exit_signal = 1; }


int main(int argc, char *argv[]) {

	unsigned int i, time, stattimes, s_sum = 0, m_sum = 0, l_sum = 0;
	size_t shmid;
	struct sigaction sa;
	FILE *fp;
	SharedMemory *shared_mem, *shm1;

	/* convert it to integer */
	shmid = atoi(argv[5]);

	time = atoi(argv[1]);
	stattimes = atoi(argv[3]);

	/* attach to shared segment */
	shared_mem = (SharedMemory *) shmat(shmid, (void *) 0, 0);
	if( (SharedMemory *) shared_mem == (void *) -1) 
		perror("shmat");

	memset(&sa, '\0', sizeof sa);
	/* function to be executed when a SIGINT signal is caught is sig_handler */
	sa.sa_handler = sig_handler;
	/* block every other signal when running sig_handler */
	sigfillset(&sa.sa_mask);
	if(sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction");
		exit(-1);
	}	

	sem_t *mutex = sem_open("/mutex", 0);

	fp = fopen("monitor_stats.txt", "w");
	if(fp == NULL) {
		fprintf(stderr, "File could not be opened!\n");
		exit(EXIT_FAILURE);
	}

	shm1 = (SharedMemory *)shared_mem;
	shm1->s_array = (small_spaces *) ( (char *)shared_mem + sizeof(SharedMemory) );
	shm1->m_array = (medium_spaces *) ( (char *)shm1->s_array + shared_mem->s_capacity * sizeof(small_spaces) );
	shm1->l_array = (large_spaces *) ( (char *)shm1->m_array + shared_mem->m_capacity * sizeof(medium_spaces) );



	while(exit_signal == 0) {

		sleep(time);

		/* print the port's current state */
		sem_wait(mutex);
			for(i = 0; i < shared_mem->s_capacity; i++) {

				if(shm1->s_array[i].vessel_status == idle) 
					fprintf(fp, "\nSmall Space %d:\nVessel name:%s\nVessel Type:%d\nArrival Time:%ld\n", i + 1, shm1->s_array[i].name, \
													shm1->s_array[i].vessel_type, \
													shm1->s_array[i].park_time);
			}
			for(i = 0; i < shared_mem->m_capacity; i++) {

				if(shm1->m_array[i].vessel_status == idle) 
					fprintf(fp, "\nMedium Space %d:\nVessel name:%s\nVessel Type:%d\nArrival Time:%ld\n", i + 1, shm1->m_array[i].name, \
													shm1->m_array[i].vessel_type, \
													shm1->m_array[i].park_time);
			}
			for(i = 0; i < shared_mem->l_capacity; i++) {

				if(shm1->l_array[i].vessel_status == idle) 
					fprintf(fp, "\nLarge Space %d:\nVessel name:%s\nVessel Type:%d\nArrival Time:%ld\n", i + 1, shm1->l_array[i].name, \
													shm1->l_array[i].vessel_type, \
													shm1->l_array[i].park_time);
			}
			fflush(fp);
		sem_post(mutex);

		sleep(stattimes - time);

		/* print time statistics + total money collected */
		sem_wait(mutex);
			/* avg waiting time for small vessels */
			for(i = 0; i < 30; i++) 
				s_sum += shared_mem->s_waitingtime[i];
			fprintf(fp, "\nAverage waiting time for small vessels: %ld\n", (time_t) s_sum / shared_mem->swt_index);
			/* avg waiting time for medium vessels */
			for(i = 0; i < 30; i++) 
				m_sum += shared_mem->m_waitingtime[i];
			fprintf(fp, "Average waiting time for medium vessels: %ld\n", (time_t) m_sum / shared_mem->mwt_index);
			/* avg waiting time for large vessels */
			for(i = 0; i < 30; i++) 
				l_sum += shared_mem->l_waitingtime[i];
			fprintf(fp, "Average waiting time for large vessels: %ld\n", (time_t) l_sum / shared_mem->lwt_index);
			/* total avg waiting time */
			fprintf(fp, "Average waiting time: %ld\n", (time_t) (s_sum + m_sum + l_sum) / (shared_mem->swt_index + shared_mem->mwt_index + shared_mem->lwt_index));
			/* total money collected */
			fprintf(fp, "Port's money: %d\n", shared_mem->total_money);
		sem_post(mutex);
		fflush(fp);
	}

	fclose(fp);
	sem_close(mutex);

	if( (shmdt( (void *) shared_mem) ) == -1)
		perror("detach");

	exit(EXIT_SUCCESS);
}
