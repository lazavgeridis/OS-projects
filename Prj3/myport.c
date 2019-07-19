/* myport.c */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>

#include "SharedMem.h"
#include "myportInterface.h"

#define MAXLONG 2147483647
#define VESSELS 20		/* number of vessel processes to be created */


volatile sig_atomic_t exit_signal = 0;

void sig_handler(int signum) { exit_signal = 1; }

int main(int argc, char *argv[]) {
	
	int j, shmid, status;
	unsigned int i, s_cap, m_cap, l_cap, s_cost, m_cost, l_cost; 
	char buffer[2], name_buffer[10], shmid_buffer[10];
	struct sigaction sa;
	FILE *fp;
	SharedMemory *shared_mem;



	if(argc != 5) {
		fprintf(stderr, "\nUsage:\n\t./myport -l configfile -c charges\n\n");
		exit(EXIT_FAILURE);
	}



	/* function to be executed when a SIGINT signal is caught, is sig_handler */
	sa.sa_handler = sig_handler;
	/* block every other signal when running sig_handler */
	sigfillset(&sa.sa_mask);
	if(sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction");
		exit(-2);
	}


	/* create shared memory segment */
	shmid = shmget(IPC_PRIVATE, sizeof(SharedMemory), 0666);
	if(shmid ==  -1)  perror("shmget");


	sprintf(shmid_buffer, "%d", shmid);

	shared_mem = (SharedMemory *) shmat(shmid, (void *) 0, 0); 
	if( (SharedMemory *) shared_mem == (void *)-1) 	perror("shmat");

	/* open the configuration file:
	 * it lists the number of parking spaces for each vessel type(small, medium, large)
	 */
	fp = fopen(argv[2], "r");
	if(fp == NULL) {
		fprintf(stderr, "Configuration file could not be opened!\n");
		exit(EXIT_FAILURE);
	}
	
	/* acquire the 3 capacities */
	fscanf(fp, "%s %d", buffer, &s_cap);  
	fscanf(fp, "%s %d", buffer, &m_cap);  
	fscanf(fp, "%s %d", buffer, &l_cap);  
	fclose(fp); /* close config file */

	/* open the charges file:
	 * it provides the cost for the 3 types of parking spaces(small, medium, large)
	 */
	fp = fopen(argv[4], "r");
	if(fp == NULL) {
		fprintf(stdin, "Charges file could not be opened!\n");
		exit(EXIT_FAILURE);
	}

	/* acquire the 3 costs */
	fscanf(fp, "%s %d", buffer, &s_cost);  
	fscanf(fp, "%s %d", buffer, &m_cost);  
	fscanf(fp, "%s %d", buffer, &l_cost);  
	fclose(fp); /* close charges file */


	/* allocate memory for the struct arrays */
	//shared_mem->s_array = malloc(s_cap * sizeof(small_spaces) );
	//shared_mem->m_array = malloc(m_cap * sizeof(medium_spaces) );
	//shared_mem->l_array = malloc(l_cap * sizeof(large_spaces) );

	/* initializations */

	/* 3 struct arrays */
	for( i = 0; i < s_cap; i++) { 
		shared_mem->s_array[i].space_type = small;
		shared_mem->s_array[i].vessel_status = departed; /* in other words this parking space is empty */
		shared_mem->s_array[i].cost = s_cost;
	}
	for( i = 0; i < m_cap; i++)  {
		shared_mem->m_array[i].space_type = medium;
		shared_mem->m_array[i].vessel_status = departed;
		shared_mem->m_array[i].cost = m_cost;
	}

	for( i = 0; i < l_cap; i++)  {
		shared_mem->l_array[i].space_type = large;
		shared_mem->l_array[i].vessel_status = departed;
		shared_mem->l_array[i].cost = l_cost;
	}


	for( i = 0; i < 30; ++i) {
		shared_mem->sm_array[i] = MAXLONG;
		shared_mem->s_waitingtime[i] = 0;
	}
	for( i = 0; i < 30; ++i) {
		shared_mem->sl_array[i] = MAXLONG;
	}
	for( i = 0; i < 30; ++i) {
		shared_mem->ml_array[i] = MAXLONG;
		shared_mem->m_waitingtime[i] = 0;
	}
	for( i = 0; i < 30; ++i) {
		shared_mem->lrg_array[i] = MAXLONG;
		shared_mem->l_waitingtime[i] = 0;
	}

	/* semaphores */
	sem_t *mutex = sem_open("/mutex", O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 1);
	sem_t *port_mutex = sem_open("/port_mutex", O_CREAT|O_EXCL, S_IRUSR | S_IWUSR, 1); 	
	sem_t *port_master = sem_open("/port_master", O_CREAT|O_EXCL, S_IRUSR | S_IWUSR, 0);
	sem_t *port_master_exit = sem_open("/port_master_exit", O_CREAT|O_EXCL, S_IRUSR | S_IWUSR, 0); 	
	sem_t *pending_vessel = sem_open("/pending_vessel", O_CREAT|O_EXCL, S_IRUSR | S_IWUSR, 0); 	
	sem_t *pre_park = sem_open("/pre_park", O_CREAT|O_EXCL, S_IRUSR | S_IWUSR, 0); 	
	sem_t *examine = sem_open("/examine", O_CREAT|O_EXCL, S_IRUSR | S_IWUSR, 0); 	
	sem_t *examine_entering = sem_open("/examine_entering", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
	sem_t *examine_exiting = sem_open("/examine_exiting", O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0);
	sem_t *small_medium_queue = sem_open("/small_medium_queue", O_CREAT|O_EXCL, S_IRUSR | S_IWUSR, 0); 	
	sem_t *small_large_queue = sem_open("/small_large_queue", O_CREAT|O_EXCL, S_IRUSR | S_IWUSR, 0); 	
	sem_t *medium_large_queue = sem_open("/medium_large_queue", O_CREAT|O_EXCL, S_IRUSR | S_IWUSR, 0); 	
	sem_t *large_queue = sem_open("/large_queue", O_CREAT|O_EXCL, S_IRUSR | S_IWUSR, 0); 	

	/* close all the semaphores */
	sem_close(mutex);
	sem_close(port_master);
	sem_close(port_master_exit);
	sem_close(port_mutex);
	sem_close(pending_vessel);
	sem_close(pre_park);
	sem_close(examine);
	sem_close(examine_entering);
	sem_close(examine_exiting);
	sem_close(small_medium_queue);
	sem_close(small_large_queue);
	sem_close(medium_large_queue);
	sem_close(large_queue);

	/* int variables */
	shared_mem->s_capacity = s_cap;
	shared_mem->m_capacity = m_cap;
	shared_mem->l_capacity = l_cap;
	shared_mem->s_spaces = s_cap;
	shared_mem->m_spaces = m_cap;
	shared_mem->l_spaces = l_cap;

	shared_mem->sm_count = 0;
	shared_mem->sl_count = 0;
	shared_mem->ml_count = 0;
	shared_mem->l_count = 0;

	shared_mem->ready_to_enter = 0;
	shared_mem->ready_to_exit = 0;
	shared_mem->waiting_pm = 0;
	shared_mem->sm_index = 0;
	shared_mem->sl_index = 0;
	shared_mem->ml_index = 0;
	shared_mem->l_index = 0;
	shared_mem->swt_index = 0;
	shared_mem->mwt_index = 0;
	shared_mem->lwt_index = 0;
	shared_mem->total_money = 0;

	strcpy(shared_mem->public_ledger,"past_public_ledger.txt");
	strcpy(shared_mem->logfile,"logfile.txt");


	initTime();	/* srand */



	/********** Now create the different processes **********/

	/* Port_master process */
	char **pmaster_argv;
	pmaster_argv = createPmasterArgv(shmid_buffer);

	for(j = 0; j < 2; ++j)
		printf("%s ", pmaster_argv[j]);
	putchar('\n');

	if(fork() == 0)
		execvp("./port_master", pmaster_argv);	/* child process executes the port_master binary */

	/* free the argument array */
	for(j = 0; j < 2; ++j) 
		free(pmaster_argv[j]);
	free(pmaster_argv);


	/* Monitor process */
	char **monitor_argv = createMonitorArgv(shmid_buffer);

	if(fork() == 0)
		execvp("./monitor", monitor_argv);	/* child process executes the monitor binary */

	/* free the argument array */
	for(j = 0; j < 6; j++) {
		printf("%s ", monitor_argv[j]);
		free(monitor_argv[j]);
	}
	free(monitor_argv);


	/* Vessel processes */
	char **vessel_argv;
	for(i = 0; i < VESSELS; ++i) {

		sprintf(name_buffer, "Vessel_%d", i);
		vessel_argv = createVesselArgv(name_buffer, shmid_buffer);

		for(j = 0; j < 12; ++j) {
			printf("%s ", vessel_argv[j]);
		}
		putchar('\n');

		if(fork() == 0)	
			execvp("./vessel" , vessel_argv); /* each child process executes the vessel binary */

		for(j = 0; j < 12; ++j) {
			free(vessel_argv[j]);
		}
		free(vessel_argv);
		vessel_argv = NULL;
	}

	/********** Processes were created **********/


	/* After forking off all its children processes(vessels, port master, monitor) 
	 * myport should wait for them to finish execution 
	 */
	while( ((wait(&status)) > 0) && (exit_signal == 0) );

	
	/* remove all the semaphores from the system */
	sem_unlink("/mutex");
	sem_unlink("/port_master");
	sem_unlink("/examine");
	sem_unlink("/examine_entering");
	sem_unlink("/examine_exiting");
	sem_unlink("/pre_park");
	sem_unlink("/port_master_exit");
	sem_unlink("/port_mutex");
	sem_unlink("/pending_vessel");
	sem_unlink("/small_medium_queue");
	sem_unlink("/small_large_queue");
	sem_unlink("/medium_large_queue");
	sem_unlink("/large_queue");

	/* remove shared memory shegment */
	if( (shmctl(shmid, IPC_RMID, 0) ) == -1)
	       perror("Removal");

	exit(EXIT_SUCCESS);
}
