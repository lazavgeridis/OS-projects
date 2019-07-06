/* searcher.c */

#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "record.h"

#define STATSIZE 15


int main(int argc, char *argv[]) {

	FILE *fpb;
	int  fd, i, searcher_num, records_share, numOfrecords, numOfsearchers, nwrite, start = 0, end = 0, sum = 0;
	char record_str[RECSIZE] = { '\0' }, time_str[STATSIZE] = { '\0' };
	struct timeval t_start, t_end;
	MyRecord rec;


	/* start timer */
	gettimeofday(&t_start, NULL);

	numOfrecords = atoi(argv[5]);
	numOfsearchers = atoi(argv[6]);
	searcher_num = atoi(argv[7]);


	/* first, open the binary file for reading */
	fpb = fopen(argv[1], "rb");
	if(fpb == NULL) {
		fprintf(stderr, "Cannot open binary data file!\n");
		exit(EXIT_FAILURE);
	}


	/* next open the named pipe file only for writing, which was provided as an argument by the sm parent process */
	if( (fd = open(argv[8], O_WRONLY) ) < 0) {
		perror("searcher: fifo open error");
		exit(-2);
	}


	if(strcmp(argv[4] , "-s") != 0) {

		/* -s flag was NOT among the arguments */
		/* set the searching range for the searcher process, which is [start:end] */
		records_share = numOfrecords / numOfsearchers;	/* k / 2^h */
		start = ( (searcher_num - 1) * records_share) + 1;
		end = searcher_num * records_share;
		if( (searcher_num == numOfsearchers) && (end < numOfrecords) ) {
			end = numOfrecords;
		}
	}
	else {
		/* -s flag WAS among the arguments */
		/* produce the sum */
		for(i = 1; i <= numOfsearchers; ++i) 
			sum += i;
		records_share = numOfrecords / sum;
		for(i = 0; i < searcher_num; ++i) 
			start += i * records_share;
		start += 1;
		for(i = 0; i <= searcher_num; ++i) 
			end += i * records_share;
		if( (searcher_num == numOfsearchers) && (end < numOfrecords) ) {
			end = numOfrecords;
		}
	}


	/* place the file pointer at the record starting at the start variable */
	if(fseek(fpb, (start - 1) * sizeof(MyRecord), SEEK_SET ) == -1) {
		perror("searcher: fseek");
		exit(-3);
	}
	
	for(i = start - 1; i < end; ++i) {	/* searcher examines its "share" of the given range of records */

		/* read each record and store it in a struct */
		fread(&rec, sizeof(rec), 1, fpb);

		/* convert the struct data to a string */
		sprintf(record_str, "%ld %s %s  %s %d %s %s %-9.2f\n", \
			rec.custid, rec.LastName, rec.FirstName, \
			rec.Street, rec.HouseID, rec.City, rec.postcode, \
			rec.amount);

		/* check if an occurence of the string pattern is present in the string record_str */
		if(strstr(record_str, argv[3]) != NULL) {	/* pattern found! */

			/* write the string to the named pipe */
			if(  (nwrite =  write(fd, record_str, sizeof(record_str)) ) == -1) {
				perror("Error when writing a record to the pipe");
				exit(-4);
			}

		}
	}

	
	/* close the file pointer used for reading the binary file */	
	fclose(fpb);

	/* stop timer */
	gettimeofday(&t_end, NULL);

	/* calculate searcher's execution time */
	double time_taken;

	time_taken = (t_end.tv_sec - t_start.tv_sec) * 1e6;
	time_taken = (time_taken + (t_end.tv_usec - t_start.tv_usec)) * 1e-6;

	/* convert time to string and write it to the named pipe of the parent */
	sprintf(time_str, "T %f" , time_taken);
	if( (nwrite = write(fd, time_str, sizeof(time_str))) == -1) {
		perror("searcher: error when writing a time stat to the pipe");
		exit(-5);
	}

	kill(atoi(argv[9]), SIGUSR2);	/* before a searcher process terminates, it sends a SIGUSR2 signal to the root process */

	exit(EXIT_SUCCESS);
}
