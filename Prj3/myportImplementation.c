/* myportImplementation.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define VESSELARGS 13
#define PMASTERARGS 3
#define MONITORARGS 7


void initTime() {
	srand(time(NULL));
}

char **createVesselArgv(char buff[10], char shmid[10]) {

	char buff1[2], buff2[2], buff3[4], buff4[5];
	char **argv_arr;

	argv_arr = malloc(VESSELARGS * sizeof(char *) );

	unsigned int type = rand() % 3;	/* generate small(0), medium(1) or large(2) vessel */ 
	sprintf(buff1, "%d", type);
	unsigned int postype;

	if(type == 0) {	/* small vessel was generated */
		postype = rand() % 2 + 1; /* upgrade to medium or large */
		sprintf(buff2, "%d", postype);
	}
	else {
		if(type == 1) { /* medium vessel was generated */
			postype = 2; /* can only upgrade to large */
			sprintf(buff2, "%d", postype);
		}
		else { 	/* large vessel was generated */
			postype = 2;	/* since -u flag is necessary set postype to large for the sake of upgrading */
			sprintf(buff2, "%d", postype);
		}
	}

	unsigned int mantime = ( (rand() % 4) + 1) * 2;
	sprintf(buff3, "%d", mantime);
	unsigned int parkperiod = ( (rand() % 8) + 1) * 3;
	sprintf(buff4, "%d", parkperiod); 

	/* pass -t */
	argv_arr[0] = malloc(strlen("-t") + 1);
	strcpy(argv_arr[0], "-t");
	/* pass type */
	argv_arr[1] = malloc(strlen(buff1) + 1);
	strcpy(argv_arr[1], buff1);
	/* pass -u */
	argv_arr[2] = malloc(strlen("-u") + 1);
	strcpy(argv_arr[2], "-u");
	/* pass postype */
	argv_arr[3] = malloc(strlen(buff2) + 1);
	strcpy(argv_arr[3], buff2);
	/* pass -p */
	argv_arr[4] = malloc(strlen("-p") + 1);
	strcpy(argv_arr[4], "-p");
	/* pass parkperiod */
	argv_arr[5] = malloc(strlen(buff4) + 1);
	strcpy(argv_arr[5], buff4);
	/* pass -m */
	argv_arr[6] = malloc(strlen("-m") + 1);
	strcpy(argv_arr[6], "-m");
	/* pass mantime */
	argv_arr[7] = malloc(strlen(buff3) + 1);
	strcpy(argv_arr[7], buff3);
	/* pass -s */
	argv_arr[8] = malloc(strlen("-s") + 1);
	strcpy(argv_arr[8], "-s");
	/* pass shmid */
	argv_arr[9] = malloc(strlen(shmid) + 1);
	strcpy(argv_arr[9], shmid);
	/* pass -n */
	argv_arr[10] = malloc(strlen("-n") + 1);
	strcpy(argv_arr[10], "-n");
	/* pass name */
	argv_arr[11] = malloc(strlen(buff) + 1);
	strcpy(argv_arr[11], buff);
	/* append null */
	argv_arr[12] = NULL;

	return argv_arr;
}


char **createPmasterArgv(char shmid[10]) {

	char **argv_arr;

	argv_arr = malloc(PMASTERARGS * sizeof(char *) );
	argv_arr[0] = malloc(strlen("-s") + 1);
	strcpy(argv_arr[0], "-s");
	argv_arr[1] = malloc(strlen(shmid) + 1);
	strcpy(argv_arr[1], shmid);
	argv_arr[2] = NULL;

	return argv_arr;
}


char **createMonitorArgv(char shmid[10]) {

	char **argv_arr;
	char buffer[3];
	unsigned int times, stattimes;

	argv_arr = malloc(MONITORARGS * sizeof(char *) );

	/* copy -d */
	argv_arr[0] = malloc(strlen("-d") + 1);
	strcpy(argv_arr[0], "-d");
	/* times */
	times = 20;
	sprintf(buffer, "%d", times);
	argv_arr[1] = malloc(strlen(buffer) + 1);
	strcpy(argv_arr[1], buffer);
	/* copy -t */
	argv_arr[2] = malloc(strlen("-t") + 1);
	strcpy(argv_arr[2], "-t");
	/* stattimes */
	stattimes = 35;
	sprintf(buffer, "%d", stattimes);
	argv_arr[3] = malloc(strlen(buffer) + 1);
	strcpy(argv_arr[3], buffer);
	/* copy -s */
	argv_arr[4] = malloc(strlen("-s") + 1);
	strcpy(argv_arr[4], "-s");
	/* copy shmid */
	argv_arr[5] = malloc(strlen(shmid) + 1);
	strcpy(argv_arr[5], shmid);
	/* append NULL */
	argv_arr[6] = NULL;

	return argv_arr;
}
