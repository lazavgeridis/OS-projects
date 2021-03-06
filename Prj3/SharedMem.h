#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#define MAXLONG 2147483647

#include <time.h>

typedef enum {small, medium, large} type;
typedef enum {idle, departed} status;
typedef enum {False, True} boolean;


typedef struct spaces {
	char name[10];	
	time_t park_time;
	time_t depart_time;
	unsigned int cost;
	type space_type;
	type vessel_type;
	status vessel_status;
}parking_spaces;

typedef struct vessel_slot1 {
	char name[10];	
	unsigned int park_period;
	unsigned int mantime;
	type primary_type;
	type upgrade_type;
	type park_space;
}vessel_info;

typedef struct vessel_slot2 {
	char name[10];
	time_t left_space;
	unsigned int v_cost;
	type parkspace;
	type primary_type;
	type upgrade_type;
}exit_info;

typedef struct shared_memory {

	/* each array represents a type of parking spaces(small, medium, large) */
	parking_spaces *array;

	/* used for vessel-port master communication */	
	int ready_to_enter;
	int ready_to_exit;
	int waiting_pm;
	char public_ledger[23];
	char logfile[12];
	vessel_info info;
	exit_info exit_info;
	type space_to_park;

	/* more variables */
	unsigned int s_capacity;
	unsigned int m_capacity;
	unsigned int l_capacity;
	unsigned int s_cost;
	unsigned int m_cost;
	unsigned int l_cost;
	unsigned int total_money;
	int s_spaces;
	int m_spaces;
	int l_spaces;
	int sm_count;
	int sl_count;
	int ml_count;
	int l_count;

	/* used for storing the waiting times of the vessels */
	time_t s_waitingtime[30];
	time_t m_waitingtime[30];
	time_t l_waitingtime[30];
	int swt_index;
	int mwt_index;
	int lwt_index;

	/* used for storing the time at which a vessel got blocked */
	long sm_array[30];
	long sl_array[30];
	long ml_array[30];
	long lrg_array[30];
	int sm_index;
	int sl_index;
	int ml_index;
	int l_index;

}SharedMemory;

#endif
