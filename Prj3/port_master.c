/* port_master.c */

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

#define MAXLONG 2147483647

volatile sig_atomic_t exit_signal = 0;
void sig_handler(int signum) { exit_signal = 1; }

int main(int argc, char *argv[]) {

	size_t shmid;
	long key, min_arr[4], min1, min2, min3, min4, total_min;
	unsigned int i;
	int j, counter = 0;
	struct sigaction sa;
	FILE *fp1;
	SharedMemory *shared_mem;
	type flag;


	shmid = atoi(argv[1]);

	/* attach to shared segment */
	shared_mem = (SharedMemory *) shmat(shmid, (void *) 0, 0);
	if( (SharedMemory *) shared_mem == (void *) -1) 
		perror("shmat");

	/* function to be executed when a SIGINT signal is caught is sig_handler */
	sa.sa_handler = sig_handler;
	/* block every other signal when running sig_handler */
	sigfillset(&sa.sa_mask);
	if(sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction");
		exit(-1);
	}


	/* open all the named semaphores */
	sem_t *mutex = sem_open("/mutex", 0);
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

	fp1 = fopen(shared_mem->public_ledger, "w");
	if(fp1 == NULL) {
		fprintf(stderr, "Public ledger could not be opened!\n");
		exit(EXIT_FAILURE);
	}



	while(exit_signal == 0) {


		/* port master "sleeps" until a pending vessel shows up - avoid busy waiting */
		sem_wait(pending_vessel);


		/* if there are vessels waiting to exit the port */
		if(shared_mem->ready_to_exit > 0) {

			sem_post(port_master_exit);

			sem_wait(examine_exiting); 
			
			/* find where the vessel was parked, and increment this type of parking spaces */
			if(shared_mem->exit_info.parkspace == small) {

				for(i = 0; i < shared_mem->s_capacity; ++i) { 
					/* always check if the space is occupied by an idle vessel, because if it is not occupied 
					 * we may compare the vessel's name with an uninitialized string 
					 */
					if(shared_mem->s_array[i].vessel_status == idle)
						if(strcmp(shared_mem->s_array[i].name, shared_mem->exit_info.name) == 0)  {
							sem_wait(mutex);

							shared_mem->s_array[i].vessel_status = departed;
							shared_mem->s_array[i].depart_time = shared_mem->exit_info.left_space;
							++shared_mem->s_spaces;

							sem_post(mutex);
							flag = small;
							break;

						}
				}

				
			}
			else {
				if(shared_mem->exit_info.parkspace == medium) {


					for(i = 0; i < shared_mem->m_capacity; ++i) { 
						if(shared_mem->m_array[i].vessel_status == idle)
							if(strcmp(shared_mem->m_array[i].name, shared_mem->exit_info.name) == 0)  {
								sem_wait(mutex);

								shared_mem->m_array[i].vessel_status = departed;
								shared_mem->m_array[i].depart_time = shared_mem->exit_info.left_space;
								++shared_mem->m_spaces;

								sem_post(mutex);
								flag = medium;
								break;
							}
					}

				}
				else	{
					/* if it reached that far, vessel's type in exit info must be large */
					for(i = 0; i < shared_mem->l_capacity; ++i)  {
						if(shared_mem->l_array[i].vessel_status == idle)
							if(strcmp(shared_mem->l_array[i].name, shared_mem->exit_info.name) == 0)  {
								sem_wait(mutex);

								shared_mem->l_array[i].vessel_status = departed;
								shared_mem->l_array[i].depart_time = shared_mem->exit_info.left_space;
								++shared_mem->l_spaces;

								sem_post(mutex);
								flag = large;
								break;
							}
					}
				}
			}

			if(flag == small)
				fprintf(fp1, "Vessel Name:%s\nArrival Time:%ld\nParking Space:Small\nVessel Type:Small\nVessel Status:departed\n"
						"Total Cost:%d\nDeparture Time:%ld\n\n", shared_mem->exit_info.name, shared_mem->s_array[i].park_time, \
											shared_mem->exit_info.v_cost, shared_mem->s_array[i].depart_time);
			else
				if(flag == medium) {
					if(shared_mem->exit_info.primary_type == small)
				      		fprintf(fp1, "Vessel Name:%s\nArrival Time:%ld\nParking Space:Medium\nVessel Type:Small\nVessel Status:departed\n"
								"Total Cost:%d\nDeparture Time:%ld\n\n", shared_mem->exit_info.name, \
													shared_mem->m_array[i].park_time, \
												shared_mem->exit_info.v_cost, shared_mem->m_array[i].depart_time);

					else { 	
				      		fprintf(fp1, "Vessel Name:%s\nArrival Time:%ld\nParking Space:Medium\nVessel Type:Medium\n"
								"Vessel Status:departed\nTotal Cost:%d\nDeparture Time:%ld\n\n", shared_mem->exit_info.name, \
							shared_mem->m_array[i].park_time, shared_mem->exit_info.v_cost, shared_mem->m_array[i].depart_time);
					}
				}
				else {
					/* flag = large */
					if(shared_mem->exit_info.primary_type == small)
				      		fprintf(fp1, "Vessel Name:%s\nArrival Time:%ld\nParking Space:Large\nVessel Type:Small\nVessel Status:departed\n"
								"Total Cost:%d\nDeparture Time:%ld\n\n", shared_mem->exit_info.name, \
													shared_mem->l_array[i].park_time, \
												shared_mem->exit_info.v_cost, shared_mem->l_array[i].depart_time);
					else
						if(shared_mem->exit_info.primary_type == medium)
				      			fprintf(fp1, "Vessel Name:%s\nArrival Time:%ld\nParking Space:Large\nVessel Type:Medium\n"
								"Vessel Status:departed\nTotal Cost:%d\nDeparture Time:%ld\n\n", shared_mem->exit_info.name, \
																shared_mem->l_array[i].park_time, \
												shared_mem->exit_info.v_cost, shared_mem->l_array[i].depart_time);
						else /* primary type is large */
				      			fprintf(fp1, "Vessel Name:%s\nArrival Time:%ld\nParking Space:Large\nVessel Type:Large\n"
								"Vessel Status:departed\nTotal Cost:%d\nDeparture Time:%ld\n\n", shared_mem->exit_info.name, \
							       shared_mem->l_array[i].park_time, shared_mem->exit_info.v_cost, shared_mem->l_array[i].depart_time);
				}

			fflush(fp1);

			/* If the type of parking spaces that was incremented was previously 0, port master should perform V() on a semaphore queue 
			 * which includes this vessel type and also includes the vessel with the longest waiting time */

			/* if one small parking space became available, while previously all the small spaces were occupied: */
			if( (flag == small) && (shared_mem->s_spaces == 1) ) {

				min1 = MAXLONG;
				min2 = MAXLONG;

				/* candidate semaphore queues are: small_medium_queue, small_large_queue
				 * of course these semaphores should have some vessels blocked on them 
				 */
				if(shared_mem->sm_count > 0) {
					
					/* small_medium_queue semaphore has vessels blocked on it */
					/* find the longest waiting time(= min element) */
					min1 = shared_mem->sm_array[0];
					for(i = 0; i < 30; ++i) {
						if(shared_mem->sm_array[i] < min1)
							min1 = shared_mem->sm_array[i];
					}
				}
				if(shared_mem->sl_count > 0) {

					/* small_large semaphore queue has vessels blocked on it */
					/* find the longest waiting time(= min element) */
					min2 = shared_mem->sl_array[0];
					for(i = 0; i < 30; ++i) {
						if(shared_mem->sl_array[i] < min2)
							min2 = shared_mem->sl_array[i];
					}

				}
				if( (min1 != MAXLONG) || (min2 != MAXLONG) ) {
					/* at this point port master should compare the 2 min's and decide on which semaphore queue it will perform V() on */
					if(min1 < min2) {

						/* V(small_medium) */
						sem_wait(mutex);

						sem_post(small_medium_queue);
						--shared_mem->sm_count;
						shared_mem->space_to_park = small;
						--shared_mem->s_spaces;
						shared_mem->last_decr = small;

						sem_post(mutex);
					}
					else {
						/* V(small_large) */
						sem_wait(mutex);

						sem_post(small_large_queue);
						--shared_mem->sl_count;
						shared_mem->space_to_park = small;
						--shared_mem->s_spaces;
						shared_mem->last_decr = small;


						sem_post(mutex);
					}
				}
			}
			else
				/* if one medium space became available, while previusly all the medium spaces were occupied */
				if( (flag == medium) && (shared_mem->m_spaces == 1) ) {

					min1 = MAXLONG;
					min2 = MAXLONG;

					/* candidate semaphore queues are: small_medium, medium_large */
					/* of course these semaphores should have some vessels blocked on them */
					if(shared_mem->sm_count > 0) {
						
						/* small_medium semaphore queue has vessels blocked on it */
						/* find the longest waiting time(= min element) */
						min1 = shared_mem->sm_array[0];
						for(i = 0; i < 30; ++i) {
							if(shared_mem->sm_array[i] < min1)
								min1 = shared_mem->sm_array[i];
						}
					}
					if(shared_mem->ml_count > 0) {

						/* medium_large semaphore queue has vessels blocked on it */
						/* find the longest waiting time(= min element) */
						min2 = shared_mem->ml_array[0];
						for(i = 0; i < 30; ++i) {
							if(shared_mem->ml_array[i] < min2)
								min2 = shared_mem->ml_array[i];
						}

					}
					if( (min1 != MAXLONG) || (min2 != MAXLONG) ) {
						/* at this point port master should compare the 2 min's and decide on which semaphore queue it will perform V() on */
						if(min1 < min2) {

							/* V(small_medium) */
							sem_wait(mutex);

							sem_post(small_medium_queue);
							--shared_mem->sm_count;
							shared_mem->space_to_park = medium;
							--shared_mem->m_spaces;
							shared_mem->last_decr = medium;


							sem_post(mutex);
						}
						else {
							/* V(medium_large) */
							sem_wait(mutex);

							sem_post(medium_large_queue);
							--shared_mem->ml_count;
							shared_mem->space_to_park = medium;
							--shared_mem->m_spaces;
							shared_mem->last_decr = medium;

							sem_post(mutex);
						}
					}
				}
				else
					/* if one large space became available, while previously all the large spaces were occupied*/
					if( (flag == large) && (shared_mem->l_spaces == 1) ) {

						min1 = MAXLONG;
						min2 = MAXLONG;
						min3 = MAXLONG;

						/* candidate semaphore queues are: small_large, medium_large, large */
						/* of course these semaphores should have some vessels blocked on them */
						if(shared_mem->sl_count > 0) {
							
							/* small_large semaphore queue has vessels blocked on it */
							/* find the longest waiting time(= min element) */
							min1 = shared_mem->sl_array[0];
							for(i = 0; i < 30; ++i) {
								if(shared_mem->sl_array[i] < min1)
									min1 = shared_mem->sl_array[i];
							}
						}
						if(shared_mem->ml_count > 0) {

							/* medium_large semaphore queue has vessels blocked on it */
							/* find the longest waiting time(= min element) */
							min2 = shared_mem->ml_array[0];
							for(i = 0; i < 30; ++i) {
								if(shared_mem->ml_array[i] < min2)
									min2 = shared_mem->ml_array[i];
							}

						}
						if(shared_mem->l_count > 0) {

							/* large semaphore queue has vessels blocked on it */
							/* find the longest waiting time(= min element) */
							min3 = shared_mem->lrg_array[0];
							for(i = 0; i < 30; ++i) {
								if(shared_mem->lrg_array[i] < min3)
									min3 = shared_mem->lrg_array[i];
							}

						}
						if( (min1 != MAXLONG) || (min2 != MAXLONG) || (min3 != MAXLONG) ) {
						/* at this point port master should compare the 3 min's and decide on which semaphore queue it will perform V() on */
							if( (min1 < min2) && (min1 < min3) ) {

								/* V(small_large) */
								sem_wait(mutex);

								sem_post(small_large_queue);
								--shared_mem->sl_count;
								shared_mem->space_to_park = large;
								--shared_mem->l_spaces;
								shared_mem->last_decr = large;

								sem_post(mutex);
							}
							else 
								if( (min2 < min1) && (min2 < min3) ) {

									/* V(medium_large) */
									sem_wait(mutex);

									sem_post(medium_large_queue);
									--shared_mem->ml_count;
									shared_mem->space_to_park = large;
									--shared_mem->l_spaces;
									shared_mem->last_decr = large;

									sem_post(mutex);
								}
								else {
									/* V(large_queue) */
									sem_wait(mutex);

									sem_post(large_queue);
									--shared_mem->l_count;
									shared_mem->space_to_park = large;
									--shared_mem->l_spaces;
									shared_mem->last_decr = large;

									sem_post(mutex);
								}
						}
					}


			/* port-master should also decrement the number of vessels that are ready to exit the port */	
			sem_wait(mutex);
			--shared_mem->ready_to_exit;
			printf("%s departed from its parking space at %ld paying a total of %d\n\n", shared_mem->exit_info.name, \
													shared_mem->exit_info.left_space, \
													shared_mem->exit_info.v_cost);
			sem_post(mutex);

			/*test*/printf("Pm: s:%d m:%d l:%d\n", shared_mem->s_spaces, shared_mem->m_spaces, shared_mem->l_spaces);

			++counter;
			printf("Number of vessels that have exited the port so far is %d\n", counter);

			printf("Pm: sm:%d sl:%d ml:%d l:%d\n", shared_mem->sm_count, shared_mem->sl_count, shared_mem->ml_count, shared_mem->l_count);
			/* increment port's money */
			shared_mem->total_money += shared_mem->exit_info.v_cost;


		}

		else
			/* If there are vessels waiting to be acknowledged by the port master and therefore wish to enter the port.
			 * The port master also checks on which semaphore queue should he perform V(), assuming there are available parking spaces */
			if(shared_mem->waiting_pm > 0) { 

				sem_post(port_master);

				sem_wait(mutex);
					--shared_mem->waiting_pm; /* decrement the number of vessels awaiting approval */
				sem_post(mutex);

				sem_wait(examine); /* port-master now waits until a vessel "files a submission" to enter the port */


				/* Check the arrival times of the vessels blocked on the semaphore "queues"(assuming there are vessels blocked on them).
				 * Then, find the semaphore "queue" that has the vessel with the longest waiting time.
				 * Assuming that waiting on a semaphore follows a fifo policy, the longest waiting time should be the first element of the array.
				 * Before performing V() on that semaphore, ensure that there are available parking spaces for the vessels waiting on it 
				 */

				if( (shared_mem->s_spaces == 0) && (shared_mem->m_spaces == 0) && (shared_mem->l_spaces == 0) ) {
					printf("\n==================== Port-Master: WARNING, NO AVAILABLE PARKING SPACES! ====================\n");
					continue;
				}


				/* find the smallest element(time) from each array: small->medium, small->large, medium->large, large */
				if(shared_mem->sm_count > 0) {
					min1 = shared_mem->sm_array[0];
					for(i = 1; i < 30; ++i) {
						if(shared_mem->sm_array[i] < min1)
							min1 = shared_mem->sm_array[i];
					}
				}
				else
					min1 = MAXLONG;

				if(shared_mem->sl_count > 0) {
					min2 = shared_mem->sl_array[0];
					for(i = 1; i < 30; ++i) {
						if(shared_mem->sl_array[i] < min2)
							min2 = shared_mem->sl_array[i];
					}
				}
				else
					min2 = MAXLONG;

				if(shared_mem->ml_count > 0) {
					min3 = shared_mem->ml_array[0];
					for(i = 1; i < 30; ++i) {
						if(shared_mem->ml_array[i] < min3)
							min3 = shared_mem->ml_array[i];
					}
				}
				else
					min3 = MAXLONG;

				if(shared_mem->l_count > 0) {
					/* find min */
					min4 = shared_mem->lrg_array[0];
					for(i = 1; i < 30; ++i) {
						if(shared_mem->lrg_array[i] < min4)
							min4 = shared_mem->lrg_array[i];
					}
				}
				else
					min4 = MAXLONG;


				/* at this point, the minimum from each array has been found 
				 * now we need to find the smallest minimum 
				 */
				min_arr[0] = min1;	/* min of sm array */
				min_arr[1] = min2;	/* min of sl array */
				min_arr[2] = min3;	/* min of ml array */
				min_arr[3] = min4;	/* min of lrg array */


				/* insertion sort - sort the minimum array */
				for(i = 1; i < 4; ++i) {

					key = min_arr[i];
					j = i - 1;
					while( (j >= 0) && (min_arr[j] > key) ) {

						min_arr[j + 1] = min_arr[j];
						j = j - 1;
					}
					min_arr[j + 1] = key;
				}
				/* min_arr sorted! */

				total_min = min_arr[0];

				/* In general, when an empty parking space is encountered, port-master must choose a vessel to "fill" it with,
				 * even if that vessel does not have the highest "priority"( = has waited the longest)
				 */ 

				if( (total_min == min1) && (total_min != MAXLONG) ) {		
					/* min1 -> small_medium_queue semaphore */
					if( (shared_mem->s_spaces > 0) || (shared_mem->m_spaces > 0) ) {

						if(shared_mem->s_spaces > 0) {
							sem_wait(mutex);
								sem_post(small_medium_queue); /* V() the semaphore */
								--shared_mem->sm_count;	      /* decrement number of vessels blocked on this semaphore */
								--shared_mem->s_spaces;	      /* decrement available small parking spaces */
								shared_mem->space_to_park = small; /* "show" to the vessel that got unblocked, its parking space */
								shared_mem->last_decr = small; /* ?????????? */
							sem_post(mutex);
						}
						else {
							sem_wait(mutex);
								sem_post(small_medium_queue);
								--shared_mem->sm_count;
								--shared_mem->m_spaces;
								shared_mem->space_to_park = medium;
								shared_mem->last_decr = medium;
							sem_post(mutex);
						}							
					}
					else {
						/* If only large parking spaces are available, candidate semaphore queues to perform V() on are:
						 * small_large, medium_large, large */
						total_min = min_arr[1];	
						if( (total_min == min2) && (total_min != MAXLONG) ) {
							sem_wait(mutex);
								sem_post(small_large_queue);
								--shared_mem->sl_count;
								--shared_mem->l_spaces;
								shared_mem->space_to_park = large;
								shared_mem->last_decr = large;
							sem_post(mutex);
						}
						else
							if( (total_min == min3) && (total_min != MAXLONG) ) {
								sem_wait(mutex);
									sem_post(medium_large_queue);
									--shared_mem->ml_count;
									--shared_mem->l_spaces;
									shared_mem->last_decr = large;
									shared_mem->space_to_park = large;
								sem_post(mutex);
							}
							else {
								if(total_min != MAXLONG) {
									sem_wait(mutex);
										sem_post(large_queue);
										--shared_mem->l_count;
										--shared_mem->l_spaces;
										shared_mem->last_decr = large;
										shared_mem->space_to_park = large;
									sem_post(mutex);
								}
							}
					}
				}
				else
					if( (total_min == min2) && (total_min != MAXLONG) ) {				/* min2->small_large_queue semaphore */
						if( (shared_mem->s_spaces > 0) || (shared_mem->l_spaces > 0 ) ) {
							if(shared_mem->s_spaces > 0) {
								sem_wait(mutex);
									sem_post(small_large_queue);
									--shared_mem->sl_count;
									--shared_mem->s_spaces;
									shared_mem->space_to_park = small;
									shared_mem->last_decr = small;
								sem_post(mutex);
							}
							else {
								sem_wait(mutex);
									sem_post(small_large_queue);
									--shared_mem->sl_count;
									--shared_mem->l_spaces;
									shared_mem->space_to_park = large;
									shared_mem->last_decr = large;
								sem_post(mutex);
							}
						}
						else {	
							/* If only medium parking spaces are available, candidate semaphore queues to perform V() on are: 
							 * small_medium_queue, medium_large_queue
							 */
							total_min = min_arr[1];		
							if( (total_min != min1) && (total_min != min3) )
								total_min = min_arr[2];

							if( (total_min == min1) && (total_min != MAXLONG)) {
								sem_wait(mutex);
									sem_post(small_medium_queue);
									--shared_mem->sm_count;
									--shared_mem->m_spaces;
									shared_mem->space_to_park = medium;
									shared_mem->last_decr = medium;
								sem_post(mutex);
							}
							else
								if( (total_min == min3) && (total_min != MAXLONG)) {
									sem_wait(mutex);
										sem_post(medium_large_queue);
										--shared_mem->ml_count;
										--shared_mem->m_spaces;
										shared_mem->space_to_park = medium;
										shared_mem->last_decr = medium;
									sem_post(mutex);
								}
						}
					}
					else {
						if( (total_min == min3) && (total_min != MAXLONG) ) {			/* min3->medium_large_queue semaphore */
							if( (shared_mem->m_spaces > 0) || (shared_mem->l_spaces > 0 ) ) {
								if(shared_mem->m_spaces > 0) {
									sem_wait(mutex);
										sem_post(medium_large_queue);
										--shared_mem->ml_count;
										--shared_mem->m_spaces;
										shared_mem->space_to_park = medium;
										shared_mem->last_decr = medium;
									sem_post(mutex);
								}
								else {
									sem_wait(mutex);
										sem_post(medium_large_queue);
										--shared_mem->ml_count;
										--shared_mem->l_spaces;
										shared_mem->space_to_park = large;
										shared_mem->last_decr = large;
									sem_post(mutex);
								}
							}
							else {	
								/* If only small parking spaces are available, candidate semaphore queues to perform V() on are: 
								 * small_medium_queue, small_large_queue
								 */
								total_min = min_arr[1];
								if( (total_min != min1) && (total_min != min2) )
									total_min = min_arr[2];
								if( (total_min == min1) && (total_min != MAXLONG)) {
									sem_wait(mutex);
										sem_post(small_medium_queue);
										--shared_mem->sm_count;
										--shared_mem->s_spaces;
										shared_mem->space_to_park = small;
										shared_mem->last_decr = small;
									sem_post(mutex);
								}
								else
									if( (total_min == min2) && (total_min != MAXLONG) ) {
										sem_wait(mutex);
											sem_post(small_large_queue);
											--shared_mem->sl_count;
											--shared_mem->s_spaces;
											shared_mem->space_to_park = small;
											shared_mem->last_decr = small;
										sem_post(mutex);
									}
							}
						}
						else {	
							if(total_min != MAXLONG) {		/* min4->large_queue semaphore */
								if(shared_mem->l_spaces > 0) {
									sem_wait(mutex);
										sem_post(large_queue);
										--shared_mem->l_spaces;
										--shared_mem->l_count;
										shared_mem->space_to_park = large;
										shared_mem->last_decr = large;
									sem_post(mutex);
								}
								else {	
									/* if only small or medium parking spaces are available, candidate semaphore queues 
									 * to perform V() on are: small_medium_queue, small_large_queue, medium_large_queue
									 */
									total_min = min_arr[1];
									if( (total_min == min1) && (total_min != MAXLONG) ) {		/* small_medium queue */
										if( (shared_mem->s_spaces > 0) || (shared_mem->m_spaces > 0) ) {
											if(shared_mem->s_spaces > 0) {
												sem_wait(mutex);
													sem_post(small_medium_queue);
													--shared_mem->sm_count;
													--shared_mem->s_spaces;
													shared_mem->space_to_park = small;
													shared_mem->last_decr = small;
												sem_post(mutex);
											}
											else {
												sem_wait(mutex);
													sem_post(small_medium_queue);
													--shared_mem->sm_count;
													--shared_mem->m_spaces;
													shared_mem->space_to_park = medium;
													shared_mem->last_decr = medium;
												sem_post(mutex);
											}
										}
									}
									if( (total_min == min2) && (total_min != MAXLONG) && (shared_mem->s_spaces > 0) ) {
										/* small_large queue */
										sem_wait(mutex);
											sem_post(small_large_queue);
											--shared_mem->sl_count;
											--shared_mem->s_spaces;
											shared_mem->space_to_park = small;
											shared_mem->last_decr = small;
										sem_post(mutex);
									}
									if( (total_min == min3) && (total_min != MAXLONG) && (shared_mem->m_spaces > 0) ) {
										sem_wait(mutex);
											sem_post(medium_large_queue);
											--shared_mem->ml_count;
											--shared_mem->m_spaces;
											shared_mem->space_to_park = medium;
											shared_mem->last_decr = medium;
										sem_post(mutex);
									}
								}
							}
						}
					}


			/*test*/printf("Pm: S:%d M:%d L:%d\n", shared_mem->s_spaces, shared_mem->m_spaces, shared_mem->l_spaces);
			/*test*/printf("Pm: SM:%d SL:%d ML:%d L:%d\n", shared_mem->sm_count, shared_mem->sl_count, shared_mem->ml_count, shared_mem->l_count);
			
			}
			else
				/* If there are vessels waiting permission from the port-master to enter the port */

				if(shared_mem->ready_to_enter > 0) {

					sem_post(pre_park);


					sem_wait(examine_entering);

					time_t vessel_arrival = time(NULL) + shared_mem->info.mantime;

					/* write to the appropriate struct array the vessel's info */
					if(shared_mem->info.park_space == small) {
						for(i = 0; i < shared_mem->s_capacity; ++i) 
							if(shared_mem->s_array[i].vessel_status == departed) {
								/* when an available space is found, copy vessel's info */
								strcpy(shared_mem->s_array[i].name, shared_mem->info.name); /* copy vessel's name */
								shared_mem->s_array[i].vessel_type = shared_mem->info.primary_type; /* copy vessel's type */
								shared_mem->s_array[i].vessel_status = idle; /* mark the space as occupied */
								shared_mem->s_array[i].park_time = vessel_arrival;/* copy vessel's arrival time at the parking space */
								break;
							}
					}
					else 
						if(shared_mem->info.park_space == medium) {
							for(i = 0; i < shared_mem->m_capacity; ++i) 
								if(shared_mem->m_array[i].vessel_status == departed) {		/* if the space is empty */
									/* when an available space is found, copy vessel's info */
									strcpy(shared_mem->m_array[i].name, shared_mem->info.name); /* copy vessel's name */
									shared_mem->m_array[i].vessel_type = shared_mem->info.primary_type;/* copy vessel's type */
									shared_mem->m_array[i].vessel_status = idle; /* mark the space as occupied */
									shared_mem->m_array[i].park_time = vessel_arrival;/* copy vessel's arrival time at the parking space */
									break;
								}
						}
						else {
							for(i = 0; i < shared_mem->l_capacity; ++i) 
								if(shared_mem->l_array[i].vessel_status == departed) {			/* if the space is empty */
									/* when an available space is found, copy vessel's info */
									strcpy(shared_mem->l_array[i].name, shared_mem->info.name); /* copy vessel's name */
									shared_mem->l_array[i].vessel_type = shared_mem->info.primary_type;/* copy vessel's type */
									shared_mem->l_array[i].vessel_status = idle; /* mark the space as occupied */
									shared_mem->l_array[i].park_time = vessel_arrival;/* copy vessel's arrival time at the parking space */
									break;
								}
						}


					sem_wait(mutex);
						printf("%s arrived at its parking space at %ld\n", shared_mem->info.name, vessel_arrival);
						--shared_mem->ready_to_enter;
					sem_post(mutex);
				}
	}

	fclose(fp1);
	/* close the named semaphores */
	sem_close(mutex);
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
