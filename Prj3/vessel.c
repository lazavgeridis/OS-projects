/* vessel.c */

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

#include "SharedMem.h"

#define MAXLONG 2147483647


int main(int argc, char *argv[]) {

	size_t shmid;
	unsigned int i, mantime, parkperiod;
	int type1, type2;
	type primary_type, upgrade_type, space_to_park;
	SharedMemory *shared_mem;


	/* convert it to integer */
	shmid = atoi(argv[9]);	

	/* convert type and postype to integers */
	/* we must locate them first */
	type1 = atoi(argv[1]);
	type2 = atoi(argv[3]);

	/* type cast to type(enum) */
	primary_type = (type) type1;
	upgrade_type = (type) type2;

	/* locate mantime and parkperiod and convert them to integers */
	mantime = atoi(argv[7]);
	parkperiod = atoi(argv[5]);

	/* attach to shared segment */
	shared_mem = (SharedMemory *) shmat(shmid, (void *) 0, 0);
	if( (SharedMemory *) shared_mem == (void *) -1) 
		perror("shmat");

	/* open all the pre-existing semaphores */
	sem_t *mutex = sem_open("/mutex", 0);
	sem_t *port_mutex = sem_open("/port_mutex", 0);
	sem_t *port_master = sem_open("/port_master", 0);
	sem_t *port_master_exit = sem_open("/port_master_exit", 0);
	sem_t *pending_vessel = sem_open("/pending_vessel", 0);
	sem_t *pre_park = sem_open("/pre_park", 0);
	sem_t *examine = sem_open("/examine", 0);
	sem_t *examine_entering = sem_open("/examine_entering", 0);
	sem_t *examine_exiting = sem_open("/examine_exiting", 0);
	sem_t *small_medium_queue = sem_open("/small_medium_queue", 0);
	sem_t *small_large_queue = sem_open("/small_large_queue", 0);
	sem_t *medium_large_queue = sem_open("/medium_large_queue", 0);
	sem_t *large_queue = sem_open("/large_queue", 0);


	/****************** acquire mutex *******************/
	sem_wait(mutex);

	/* write to shared memory - increment the number of vessels waiting to be acknowledged by the port master */
	printf("%s arrived at the port at %ld\n\n", argv[11], time(NULL) ); 	
	++shared_mem->waiting_pm;
	

	/* (1)************** V(pending_vessel) *********************(1) */
	sem_post(pending_vessel);	/* "wake up" the port-master in case he is idle */


	sem_post(mutex);		
	/******************* release mutex ******************/	

	time_t come_in = time(NULL);

	/* then wait for port-master to acknowledge the vessel's request */
	sem_wait(port_master);

	time_t queue_arrival = time(NULL);
	printf("%s: I am starting to wait on my semaphore \"queue\" on t = %ld\n", argv[11], queue_arrival);

	/* get intentionally blocked on the semaphore that describes the vessel's primary and upgrade type */
	if(primary_type == small) {
		if(upgrade_type == medium) {

			sem_wait(mutex);
				shared_mem->sm_array[shared_mem->sm_index++] = queue_arrival;	/* make public the time you started waiting */
				++shared_mem->sm_count;						/* increment the number of small->medium vessels waiting */
				printf("%s requested to enter the port at %ld\n\n", argv[11], time(NULL) ); 	
			sem_post(mutex);

			/* Signal port_master to examine the vessel's request */
			sem_post(examine);

			sem_wait(small_medium_queue);
		}
		else { /* upgrade_type = large */

			sem_wait(mutex);
				shared_mem->sl_array[shared_mem->sl_index++] = queue_arrival;	/* make public the time you started waiting */
				++shared_mem->sl_count;						/* increment the number of small->large vessels waiting */
				printf("%s requested to enter the port at %ld\n\n", argv[11], time(NULL) ); 	
			sem_post(mutex);

			/* Signal port_master to examine the vessel's request */
			sem_post(examine);

			sem_wait(small_large_queue);
		}
	}
	else {
		if(primary_type == medium) {

			sem_wait(mutex);
				shared_mem->ml_array[shared_mem->ml_index++] = queue_arrival;
				++shared_mem->ml_count;
				printf("%s requested to enter the port at %ld\n\n", argv[11], time(NULL) ); 	
			sem_post(mutex);

			/* Signal port_master to examine the vessel's request */
			sem_post(examine);

			sem_wait(medium_large_queue);
		}
		else { /* primary_type = large */

			sem_wait(mutex);
				shared_mem->lrg_array[shared_mem->l_index++] = queue_arrival;
				++shared_mem->l_count;
				printf("%s requested to enter the port at %ld\n\n", argv[11], time(NULL) ); 	
			sem_post(mutex);

			/* Signal port_master to examine the vessel's request */
			sem_post(examine);

			sem_wait(large_queue);
		}
	}


	sem_wait(mutex);
		space_to_park = shared_mem->space_to_park;	/* learn the type of parking space the port-master chose for the vessel */
	sem_post(mutex);


	/***********************************************************************/

	/* in some cases this "miscommunication" appears */
	//if( (space_to_park != primary_type) && (space_to_park != upgrade_type) ) {
	//	if(primary_type == large)	
	//		space_to_park = large;
	//}

	/************************************************************************/


	/* When a vessel exits its waiting semaphore "queue", which it joined based on its primary and upgrade types,
	 * it should find its arrival time in the queue array that contains the arrival times of every vessel waiting on this queue.
	 * Then, it should reset its value to MAXLONG, in order to prevent port-master from performing V() on a wrong semaphore queue in the future
	 */
	if(primary_type == small) {
		if(upgrade_type == medium) {	/* search in sm_array */
			for(i = 0; i < 30; ++i) { 
				if(shared_mem->sm_array[i] == queue_arrival) {
					sem_wait(mutex);
						shared_mem->sm_array[i] = MAXLONG;
						++shared_mem->ready_to_enter;
					sem_post(mutex);
					break;
				}
			}
		}
		else {				/* search in sl_array */
			for(i = 0; i < 30; ++i) {
				if(shared_mem->sl_array[i] == queue_arrival) {
					sem_wait(mutex);
						shared_mem->sl_array[i] = MAXLONG;
						++shared_mem->ready_to_enter;
					sem_post(mutex);
					break;
				}
			}
		}
	}
	else {
		if(primary_type == medium) {
			/* search in ml_array */
			for(i = 0; i < 30; ++i) {
				if(shared_mem->ml_array[i] == queue_arrival) {
					sem_wait(mutex);
						shared_mem->ml_array[i] = MAXLONG;
						++shared_mem->ready_to_enter;
					sem_post(mutex);
					break;
				}
			}
		}
		else { 	/* search in lrg_array */
			for(i = 0; i < 30; ++i) {
				if(shared_mem->lrg_array[i] == queue_arrival) {
					sem_wait(mutex);
						shared_mem->lrg_array[i] = MAXLONG;
						++shared_mem->ready_to_enter;
					sem_post(mutex);
					break;
				}
			}
		}
	}



	/* (2)************** V(pending_vessel) *****************(2) */
	sem_post(pending_vessel);


	/* another semaphore here */
	sem_wait(pre_park);

	time_t time_to_park = time(NULL);
	if(primary_type == small) {
		sem_wait(mutex);
			shared_mem->s_waitingtime[shared_mem->swt_index++] = time_to_park - come_in;
			printf("Port master gave permission for entrance to %s at %ld\n\n", argv[11], time(NULL) ); 	
		sem_post(mutex);
	}
	else {
		if(primary_type == medium) {
			sem_wait(mutex);
				shared_mem->m_waitingtime[shared_mem->mwt_index++] = time_to_park - come_in;
				printf("Port master gave permission for entrance to %s at %ld\n\n", argv[11], time(NULL) ); 	
			sem_post(mutex);
		}
		else {
			sem_wait(mutex);
				shared_mem->l_waitingtime[shared_mem->lwt_index++] = time_to_park - come_in;
				printf("Port master gave permission for entrance to %s at %ld\n\n", argv[11], time(NULL) ); 	
			sem_post(mutex);
		}
	}

	/* acquire port_mutex, in order to enter the port and park afterwards */
	sem_wait(port_mutex);

	/* also acquire regular mutex */
	sem_wait(mutex);

	/* write vessel info to vessel_slot */
	strcpy(shared_mem->info.name, argv[11]);
	shared_mem->info.primary_type = primary_type;
	shared_mem->info.upgrade_type = upgrade_type;
	shared_mem->info.park_space = space_to_park;
	shared_mem->info.park_period = parkperiod;
	shared_mem->info.mantime = mantime;
	/* now port master can copy this information to the parking space the vessel will use */
	
	/* release mutex */
	sem_post(mutex);

	/* call port master to acquire vessel's information */
	sem_post(examine_entering);


	/* manouver in order to reach your parking space */
	sleep(mantime);


	/* release port_mutex */
	sem_post(port_mutex);

	/* vessel has parked so it can now start its "activities" */
	sleep(parkperiod);


/*************************************************************************************************************/
	/* now it is time to depart */
	
	/* vessel should now be informed for the cost of its parking duration and obviously pay the fee 
	 * to do that, it should find its parking space info in the public ledger and calculate the total cost 
	 */
	time_t space_departure = time(NULL);
	printf("%s: I am leaving my parking space at %ld\n", argv[11], space_departure);
	unsigned int v_cost = 0; 

	if(space_to_park == small) {

		/* search in small spaces first */
		for(i = 0; i < shared_mem->s_capacity; ++i) {
			if(strcmp(argv[11], shared_mem->s_array[i].name) == 0) {
					v_cost = parkperiod * (shared_mem->s_array[i].cost);
					break;
			}
		}
	}
	else {
		if(space_to_park == medium) {

			/* search in medium spaces first */
			for(i = 0; i < shared_mem->m_capacity; ++i) {
				if(strcmp(argv[11], shared_mem->m_array[i].name) == 0) {
					v_cost = parkperiod * (shared_mem->m_array[i].cost);
					break;
				}
			}
		}
		/* if vessel's primary type is large */
		else {
			for(i = 0; i < shared_mem->l_capacity; ++i) {
				if(strcmp(argv[11], shared_mem->l_array[i].name) == 0) {
					v_cost = parkperiod * (shared_mem->l_array[i].cost);
					break;
				}
			}
		}
	}

	sem_wait(mutex);
		++shared_mem->ready_to_exit;
	sem_post(mutex);


	/* (3)******************* V(pending_vessel)*******************(3) */
	sem_post(pending_vessel);

	sem_wait(port_master_exit);

	sem_wait(port_mutex);

	sem_wait(mutex);

	/* vessel writes to exit_info its name,type,upgrade type,time it left its parking space, and total cost */
	strcpy(shared_mem->exit_info.name, argv[11]);
	shared_mem->exit_info.parkspace = space_to_park;
	shared_mem->exit_info.primary_type = primary_type;
	shared_mem->exit_info.upgrade_type = upgrade_type;
	shared_mem->exit_info.left_space = space_departure;
	shared_mem->exit_info.v_cost = v_cost;

	sem_post(mutex);

	/* P(examine_exiting) after vessel has written its exit info in shared memory */
	sem_post(examine_exiting);

	sleep(mantime);

	/* release port mutex */
	sem_post(port_mutex);

	printf("%s is leaving the port at %ld\n", argv[11], time(NULL));

	
	/* close the named semaphores */
	sem_close(mutex);
	sem_close(port_mutex);
	sem_close(port_master);
	sem_close(port_master_exit);
	sem_close(pending_vessel);
	sem_close(pre_park);
	sem_close(examine); 
	sem_close(examine_entering);
	sem_close(examine_exiting); 
	sem_close(small_medium_queue);
	sem_close(small_large_queue); 
	sem_close(medium_large_queue);
	sem_close(large_queue);

	if( (shmdt( (void *) shared_mem) ) == -1)
		perror("detach");

	exit(EXIT_SUCCESS);
}
