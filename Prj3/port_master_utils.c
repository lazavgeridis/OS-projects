/* port_master_utils.c */

#include <semaphore.h>

#include "SharedMem.h"


void park_small_medium(SharedMemory *shm, sem_t *mutex, sem_t *small_medium_q)
{
		
	if(shm->s_spaces > 0) {
		sem_wait(mutex);
			sem_post(small_medium_q); /* V() the semaphore */
			--shm->sm_count; /* decrement number of vessels blocked on this semaphore */
			--shm->s_spaces; /* decrement available small parking spaces */
			shm->space_to_park = small; /* "show" to the vessel that got unblocked, its parking space */
			shm->last_decr = small; /* ?????????? */
		sem_post(mutex);
	}
	else {
		sem_wait(mutex);
			sem_post(small_medium_q);
			--shm->sm_count;
			--shm->m_spaces;
			shm->space_to_park = medium;
			shm->last_decr = medium; /* ????????? */
		sem_post(mutex);
	}			
}

void park_on_large_space(SharedMemory *shm, long total_min, long small_large_min, long medium_large_min, sem_t *mutex, sem_t *small_large_q, sem_t *medium_large_q,				sem_t *large_q)
{
	if( (total_min == small_large_min) && (total_min != MAXLONG) ) {	/* V() the small_large semaphore */
		sem_wait(mutex);
			sem_post(small_large_q);
			--shm->sl_count;
			--shm->l_spaces;
			shm->space_to_park = large;
			shm->last_decr = large;
		sem_post(mutex);
	}
	else {
		if( (total_min == medium_large_min) && (total_min != MAXLONG) ) { /* V() the medium_large semaphore */
			sem_wait(mutex);
				sem_post(medium_large_q);
				--shm->ml_count;
				--shm->l_spaces;
				shm->last_decr = large;
				shm->space_to_park = large;
			sem_post(mutex);
		}
		else {
			if(total_min != MAXLONG) {	/* V() the large semaphore */
				sem_wait(mutex);
					sem_post(large_q);
					--shm->l_count;
					--shm->l_spaces;
					shm->last_decr = large;
					shm->space_to_park = large;
				sem_post(mutex);
			}
		}
	}
}	

void park_small_large(SharedMemory *shm, sem_t *mutex, sem_t *small_large_q)
{
	if(shm->s_spaces > 0) {
		sem_wait(mutex);
			sem_post(small_large_q);
			--shm->sl_count;
			--shm->s_spaces;
			shm->space_to_park = small;
			shm->last_decr = small;
		sem_post(mutex);
	}
	else {
		sem_wait(mutex);
			sem_post(small_large_q);
			--shm->sl_count;
			--shm->l_spaces;
			shm->space_to_park = large;
			shm->last_decr = large;
		sem_post(mutex);
	}
}

void park_on_medium_space(SharedMemory *shm, long total_min, long small_medium_min, long medium_large_min, sem_t *mutex, sem_t *small_medium_q, sem_t *medium_large_q)
{
	if( (total_min == small_medium_min) && (total_min != MAXLONG)) {
		sem_wait(mutex);
			sem_post(small_medium_q);
			--shm->sm_count;
			--shm->m_spaces;
			shm->space_to_park = medium;
			shm->last_decr = medium;
		sem_post(mutex);
	}
	else {
		if( (total_min == medium_large_min) && (total_min != MAXLONG)) {
			sem_wait(mutex);
				sem_post(medium_large_q);
				--shm->ml_count;
				--shm->m_spaces;
				shm->space_to_park = medium;
				shm->last_decr = medium;
			sem_post(mutex);
		}
	}
}

void park_medium_large(SharedMemory *shm, sem_t *mutex, sem_t *medium_large_q)
{
	if(shm->m_spaces > 0) {
		sem_wait(mutex);
			sem_post(medium_large_q);
			--shm->ml_count;
			--shm->m_spaces;
			shm->space_to_park = medium;
			shm->last_decr = medium;
		sem_post(mutex);
	}
	else {
		sem_wait(mutex);
			sem_post(medium_large_q);
			--shm->ml_count;
			--shm->l_spaces;
			shm->space_to_park = large;
			shm->last_decr = large;
		sem_post(mutex);
	}
}

void park_on_small_space(SharedMemory *shm, long total_min, long small_medium_min, long small_large_min, sem_t *mutex, sem_t *small_medium_q, sem_t *small_large_q)
{
	if( (total_min == small_medium_min) && (total_min != MAXLONG)) {
		sem_wait(mutex);
			sem_post(small_medium_q);
			--shm->sm_count;
			--shm->s_spaces;
			shm->space_to_park = small;
			shm->last_decr = small;
		sem_post(mutex);
	}
	else {
		if( (total_min == small_large_min) && (total_min != MAXLONG) ) {
			sem_wait(mutex);
				sem_post(small_large_q);
				--shm->sl_count;
				--shm->s_spaces;
				shm->space_to_park = small;
				shm->last_decr = small;
			sem_post(mutex);
		}
	}
}

void park_large(SharedMemory *shm, sem_t *mutex, sem_t *large_q)
{
	sem_wait(mutex);
		sem_post(large_q);
		--shm->l_count;
		--shm->l_spaces;
		shm->space_to_park = large;
		shm->last_decr = large;
	sem_post(mutex);
}

long find_min(long *arr)
{
	long min = arr[0];

	for(int i = 1; i < 30; i++)
		if(arr[i] < min)
			min = arr[i];

	return min;
}
