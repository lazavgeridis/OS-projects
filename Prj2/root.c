/* root.c */

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

#define _GNU_SOURCE


char *fifo = "fifo_root";
volatile sig_atomic_t leaf_signals = 0;

void sig_handler(int signum) { ++leaf_signals; }	/* selected SIGUSR2 signal handler */


int main(int argc, char *argv[]) {

	FILE *fpb, *fp;
	int i, h, nread, fd_child, child_status, s_flag;
	long counter = 0;
	double time_taken, var, sm_min, sm_max, sm_avg, sm0;
	char **new_argv, *token, record_str[RECSIZE];
	struct timeval t_start, t_end;
	struct sigaction sa;
	MyRecord myrec;
	pid_t child_pid;


	/* correct execution */
	if( (argc != 7) && (argc != 8) ) {	/* invalid number of arguments */
		printf("\nUsage:\n\t./myfind -h <Height> -d <Datafile> -p <Pattern> (-s)\n\n");
		exit(EXIT_FAILURE);
	}


	/* function to be executed when signal is caught is sig_handler */
	sa.sa_handler = sig_handler;
	sa.sa_flags = SA_RESTART;
	/* block every other signal when running sig_handler */
	sigfillset(&sa.sa_mask);
	if(sigaction(SIGUSR2, &sa, NULL) == -1) {
		perror("sigaction");
		exit(-3);
	}

	/* start timer */
	gettimeofday(&t_start, NULL);


	if(argc == 7)
		s_flag = 0;	/* when argument count is 7, -s flag was not given */
	else
		s_flag = 1;	/* when argument count is 8, -s flag was given */


	fp = fopen("results.txt", "w");		/* open the text file, where the root process will write the results of the query */
	if(fp == NULL) {
		printf("Could not open results file!\n");
		exit(EXIT_FAILURE);
	}	

	
	/* create a named pipe, which will be used for communication between the root and sm0 */
	if(mkfifo(fifo, 0666) == -1) {
		perror("root: mkfifo");
		exit(2);
	}

	/* copy *argv[] to a new argument array and extend it. Since an sm process will be called like this: 
	 *
	 * 			./sm -h <height> -d <data> -p <pattern> (-s) <records> <searchers> <start> <end> <fifo> <rootpid> 
	 *
	 * (+ NULL) -> create an array of [14] elements, since the executable name is not taken into account
	 */

	new_argv = malloc( 14 * sizeof(char *) );
	/* some arguments remain unchanged */
	/* locate the flags(-h, -d, -p) and copy them to new_argv along with the data provided after the flag */
	for(i = 0; i < argc; ++i) {
		/* locate -h <Height> */
		if(strcmp("-h", argv[i]) == 0) {			/* copy -h <Height> at indexes 0 and 1 respectively */
			new_argv[0] = malloc(strlen(argv[i]) + 1);
			strcpy(new_argv[0], argv[i]);
			new_argv[1] = malloc(strlen(argv[i + 1]) + 1);
			strcpy(new_argv[1], argv[i + 1]);
			break;
		}
	}

	/* convert height to integer */
	h = atoi(argv[i + 1]);

	for(i = 0; i < argc; ++i) {
		/* locate -d <Datafile> */
		if(strcmp("-d", argv[i]) == 0) {			/* copy -d <Datafile> at indexes 2 and 3 respectively */
			new_argv[2] = malloc(strlen(argv[i]) + 1);
			strcpy(new_argv[2], argv[i]);
			new_argv[3] = malloc(strlen(argv[i + 1]) + 1);
			strcpy(new_argv[3], argv[i + 1]);
			break;

		}
	}

	/* open binary file for reading */
	fpb = fopen(argv[i + 1], "rb");
	if(fpb == NULL) {
		printf("Cannot open binary datafile\n");
		exit(EXIT_FAILURE);
	}

	for(i = 0; i < argc; ++i) {
		/* locate -p <Pattern> */
		if(strcmp("-p", argv[i]) == 0) {			/* copy -p <Pattern> at indexes 4 and 5 respectively */
			new_argv[4] = malloc(strlen(argv[i]) + 1);
			strcpy(new_argv[4], argv[i]);
			new_argv[5] = malloc(strlen(argv[i + 1]) + 1);
			strcpy(new_argv[5], argv[i + 1]);
			break;
		}
	}

	/* -s was/wasn't given */
	if(s_flag) {
		new_argv[6] = malloc(strlen("-s") + 1);
		strcpy(new_argv[6], "-s");
	}
	else {
		new_argv[6] = malloc(strlen("NOT_S") + 1);
		strcpy(new_argv[6], "NOT_S");
	}

	/* at this point, our new argument array looks like this:
	 * 	
	 * 	-h <Height> -d <Datafile> -p <Pattern> -s/NOT_S	
	 * 	0	1    2	3	   4	5	6
	 */



	long lSize;
	int numOfrecords, end, numOfsearchers = 1;
	char str_rec_all[12], str_searchers[3], str_end[2], rpid_str[10], start[] = "1"; 
	const char delim[2] = " ";

	/* now calculate number of records manipulating the file pointer */
	fseek(fpb, 0, SEEK_END);
	lSize = ftell(fpb);
	rewind(fpb);
	numOfrecords = (int)lSize / sizeof(myrec);
	fclose(fpb);	/* close binary file */
	
	/* calculate the total number of searcher processes to be created */
	/* since we want to create a balanced binary tree, the total number of leaf nodes is 2^h */
	for( i = 0; i < h; ++i) 
		numOfsearchers *= 2;

	/* convert the arguments, that must be passed to sm0, to string */ 
	sprintf(str_rec_all, "%d" , numOfrecords); 	/* convert total number of records to string */
	end = numOfsearchers;
	sprintf(str_end, "%d", end);			/* convert end of the searcher range to string */
	sprintf(str_searchers, "%d" , numOfsearchers);	/* convert total number of searchers to be created to string */

	/* copy to new_argv: number of  all records  */
	new_argv[7] = malloc(strlen(str_rec_all) + 1);
	strcpy(new_argv[7], str_rec_all);
	/* copy to new_argv: number of searcher processes to be created */
	new_argv[8] = malloc(strlen(str_searchers) + 1);
	strcpy(new_argv[8], str_searchers);
	/* copy to new_argv: start of the searcher range->[1,2^h] */
	new_argv[9] = malloc(strlen(start) + 1);
	strcpy(new_argv[9], start);
	/* copy to new_argv: end of the searcher range->[1,2^h] */
	new_argv[10] = malloc(strlen(str_end) + 1);
	strcpy(new_argv[10], str_end);
	/* copy to new argv: name of the named pipe file(fifo_root) */
	new_argv[11] = malloc(strlen(fifo) + 1);
	strcpy(new_argv[11], fifo);
	/* copy to new argv: root's pid */
	sprintf(rpid_str, "%ld", (long)getpid() );
	new_argv[12] = malloc(strlen(rpid_str) + 1);
	strcpy(new_argv[12], rpid_str);
	/* append NULL to the last element of new_argv */
	new_argv[13] = NULL;
	/****** done ******/
	
	child_pid = fork();
	switch (child_pid) {

		case -1: perror("fork");				/* fork failed */
			 exit(-1);

		case 0:	 execvp("./splitter_merger", new_argv); 	/* sm0 child process */

		default:						/* root process */

			/* open the sm0 child's named pipe only for reading data */
			if( (fd_child = open(fifo, O_RDONLY) ) < 0) {
				perror("error opening child's fifo");
				exit(-2);
			}


			/* while there are bytes to read from sm0's pipe, write these bytes to a text file, so that sort() can receive a txt file as input */
			while( (nread = read(fd_child, record_str, sizeof(record_str)) ) > 0) {

				/* sm0 sent the final time statistics */
				/* in any case, the statistics record is in the form: T s_min s_max s_avg + ... */
				if(record_str[0] == 'T') {

					/* get T */
					token = strtok(record_str, delim);
					/* get searcher min */
					token = strtok(NULL, delim);
					var = atof(token);
					printf("\nMinimum execution time for a searcher was %f\n" , var);
					/* get searcher max */
					token = strtok(NULL, delim);
					var = atof(token);
					printf("Maximum execution time for a searcher was %f\n" , var);
					/* get searcher avg */
					token = strtok(NULL, delim);
					var = atof(token);
					printf("Average execution time for the searcher processes was %f\n" , var);

					if(h != 1) {		/* if the height was not 1, then min,max,avg of sm processes is also displayed + turnaround time
								   for the query + number of signals */

						/* compare sm0 execution time with sm_min and sm_max */
						/* get sm min */
						token = strtok(NULL, delim);
						sm_min = atof(token);

						/* get sm max */
						token = strtok(NULL, delim);
						sm_max = atof(token);
						/* get sm avg */
						token = strtok(NULL, delim);
						sm_avg = atof(token);
						/* get sm0 time */
						token = strtok(NULL, delim);
						sm0 = atof(token);
						if(sm0 < sm_min)
							sm_min = sm0;
						if(sm0 > sm_max)
							sm_max = sm0;
						sm_avg = (sm_avg + sm0) / 2;
						sprintf(token, "Minimum execution time for a splitter/merger was %f\n" , sm_min);
						printf("%s" , token);
						sprintf(token, "Maximum execution time for a splitter/merger was %f\n" , sm_max);
						printf("%s" , token);
						sprintf(token, "Average execution time for the splitter/merger processes was %f\n" , sm_avg);
						printf("%s", token);
						
						/* stop timer */
						gettimeofday(&t_end, NULL);
						time_taken = (t_end.tv_sec - t_start.tv_sec) * 1e6;
						time_taken = (time_taken + (t_end.tv_usec - t_start.tv_usec)) * 1e-6;
						printf("Turnaround time for the query was %f\n" , time_taken);
						printf("Received %d SIGUSR2 signals in total\n" , leaf_signals);

						break;
					}
					else {		/* if the given height was 1, only sm0 execution time is displayed + turnaround time of the query + number 
							   of signals */

						token = strtok(NULL, delim);	 
						sm0 = atof(token);
						printf("Execution time for splitter/merger 0 was %f\n" , sm0);
						gettimeofday(&t_end, NULL);
						time_taken = (t_end.tv_sec - t_start.tv_sec) * 1e6;
						time_taken = (time_taken + (t_end.tv_usec - t_start.tv_usec)) * 1e-6;
						printf("Turnaround time for the query was %f\n" , time_taken);
						printf("Received %d SIGUSR2 signals in total\n" , leaf_signals);

						break;
					}


				}
				/* write each record returned by sm0 to the text file */
				fputs(record_str, fp);
				++counter;
			}
			printf("\n->Total records found:%ld\n\n" , counter);
			wait(&child_status);	/* wait for sm0 to finish its execution */
	}

	fclose(fp);

	/* free the dynamically allocated new_argv array */
	for(i = 0; i < 13; ++i) {
		free(new_argv[i]);
	}
	free(new_argv);

	/* call sort */
	system("sort -g results.txt"); /* in the case of the bigger files, when a common pattern is given as an argument, the query returns A LOT of records */
					/* and as a result you will not be able to see the time statistics, since they are printed first */
					/* what you can do is to redirect the results to a file: sort -g results.txt > sorted_results.txt */

	/* remove the fifo_root named pipe from the system */
	unlink(fifo);

	exit(EXIT_SUCCESS);
}
