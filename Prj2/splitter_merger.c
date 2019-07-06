/* splitter_merger.c */

#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "sm_utils.h"

#define RECSIZE 90


int main(int argc, char *argv[]) 
{

	pid_t pid1, pid2, pid3, pid4;
	int i, fd_child1, fd_child2, fd_parent, nread, nwrite, status, start, end, start1, start2, end1, end2, h = atoi(argv[1]);
	double time_taken, child1_time, child2_time, s_time_min1, s_time_min2, s_time_max1, s_time_max2, s_time_avg1, s_time_avg2, sm_time1, sm_time2,
                sm_time_min1, sm_time_max1, sm_time_avg1, sm_time_min2, sm_time_max2, sm_time_avg2;
	char str_height[2], str_start1[3], str_start2[3], str_end1[3], str_end2[3], fifo1[30] = { '\0' }, fifo2[30] = { '\0' }, record_str[RECSIZE] = { '\0' }, stat_str1[38] = { '\0' }, stat_str2[65] = { '\0'};
	const char delim[2] = " ";
	char *token;
	struct timeval t_start, t_end;



	/****** start timer ******/
	gettimeofday(&t_start, NULL);


	if(h != 1) {		/* H != 1: create 2 sm children that will in turn run the splitter_merger executable */

		/* create 2 named pipes for communication between the sm process and its sm children */
		sprintf(fifo1, "fifo_sm_%d_1" , getpid() );
		if(mkfifo(fifo1, 0666) == -1) {
			perror("splitter/merger: mkfifo");
			exit(2);
		}

		sprintf(fifo2, "fifo_sm_%d_2", getpid() );
		if(mkfifo(fifo2, 0666) == -1) {
			perror("splitter/merger: mkfifo");
			exit(2);
		}

		h = h - 1;					/* update the height of the remaining tree */
		sprintf(str_height, "%d", h);			/* convert the updated height to string */

		start = atoi(argv[9]);	end = atoi(argv[10]);
		start1 = start;
		end1 = start + (end - start) / 2;
		start2 = end1 + 1;
		end2 = end;
		
		sprintf(str_start1, "%d", start1);
		sprintf(str_end1, "%d", end1);
		sprintf(str_start2, "%d", start2);
		sprintf(str_end2, "%d", end2);


		/****** create the new argument array for the 1st sm child ******/
		char **new_argv1 = create_smargs(argv, str_height, str_start1, str_end1, fifo1);

		/****** crete the new argument array for the 2nd sm child ******/
		char **new_argv2 = create_smargs(argv, str_height, str_start2, str_end2, fifo2);

		/****** argument arrays for the 2 sm children were created ******/


		/******* fork 2 children ******/

        	pid1 = fork();	/* 1st sm child */
		switch (pid1) {

			case -1: perror("fork"); /* fork failed */
				 exit(-1);

			case 0: execvp("./splitter_merger", new_argv1); /* inside 1st sm child */

			default:	/* inside sm parent process */

				/* open the named pipe file that was provided to the process as a command line argument by its parent.
				 * Sm parent process only writes to this named pipe 
				 */

				if( (fd_parent = open(argv[11], O_WRONLY)) < 0) {
					perror("sm:parent fifo open problem");
					exit(-1);
				}

				/* next, open the named pipe that the 1st child will use to write the results of its search.
				 * Sm parent process only reads from this named pipe 
				 */

				if( (fd_child1 = open(fifo1, O_RDONLY)) < 0) {
					perror("sm child 1 fifo open problem");
					exit(-2);
				}


				pid2 = fork();	/* 2nd sm child */
				switch (pid2) {

					case -1: perror("fork"); /* fork failed */
						 exit(-1);

					case 0:  execvp("./splitter_merger", new_argv2); /* inside 2nd sm child */

					default:	/* inside sm parent again */
						
						/* and then open the named pipe that the 2nd child will use to write the results of its search.
						 * Again, sm parent process only reads from this named pipe  
						 */

						if( (fd_child2 = open(fifo2, O_RDONLY)) < 0) {
							perror("sm child 2 fifo open problem");
							exit(-3);
						}

						/* while there are bytes to read from sm child 1's pipe, write these bytes to your parent's named pipe */
						while( (nread = read(fd_child1, record_str, sizeof(record_str))) > 0 ) {

						/* if we are in an sm process with height = 2 the stat record received is in the form "T min max avg sm_child_time"
						 * so if the first character of the record is T, we know that the sm child sent time stats 
						 * h was already decremented so if the old value that we entered the if statement with is 2
						 * the sm process expects a specific stats record 
						 */
                                			if(record_str[0] == 'T') {

                                        			if(h + 1 == 2) {
                                        			    /* get T */
                                        			    token = strtok(record_str, delim);
                                        			    /* get searchers min, max, avg... */
                                        			    /* min */
                                        			    token = strtok(NULL, delim);
                                        			    s_time_min1 = atof(token);
                                        			    /* max */
                                        			    token = strtok(NULL, delim);
                                        			    s_time_max1 = atof(token);
                                        			    /* avg */
                                        			    token = strtok(NULL, delim);
                                        			    s_time_avg1 = atof(token);
                                        			    /* sm child execution time*/
                                        			    token = strtok(NULL, delim);
                                        			    sm_time1 = atof(token);

                                        			    break;

                                        			}

                                        			/* slightly different stats record when h >= 3 */
                                        			/* its in the form "T s_min s_max s_avg sm_min sm_max sm_avg sm_child_time" */
                                        			else {
                                        			    /* get T */
                                        			    token = strtok(record_str, delim);
                                        			    /* get searchers min, max, avg... */
                                        			    /* min */
                                        			    token = strtok(NULL, delim);
                                        			    s_time_min1 = atof(token);
                                        			    /* max */
                                        			    token = strtok(NULL, delim);
                                        			    s_time_max1 = atof(token);
                                        			    /* avg */
                                        			    token = strtok(NULL, delim);
                                        			    s_time_avg1 = atof(token);
                                        			    /* get splitter/mergers min,max,avg */
                                        			    /* min */
                                        			    token = strtok(NULL, delim);
                                        			    sm_time_min1 = atof(token);
                                        			    /* max */
                                        			    token = strtok(NULL, delim);
                                        			    sm_time_max1 = atof(token);
                                        			    /* avg */
                                        			    token = strtok(NULL, delim);
                                        			    sm_time_avg1 = atof(token);
                                        			    /* sm child execution time*/
                                        			    token = strtok(NULL, delim);
                                        			    sm_time1 = atof(token);

                                        			    break;
                                        			}
                                			}

							if( (nwrite = write(fd_parent, record_str, sizeof(record_str))) < 0) {
								perror("sm:error while writing to parent's pipe");
								exit(1);
							}
						}

						wait(&status);		/* wait for your 1st sm child to finish its operation */

						/* while there are bytes to read from child 2's pipe, write these bytes to your parent's named pipe */
						while( (nread = read(fd_child2, record_str, sizeof(record_str))) > 0 ) {

                               				/* again if a stats record arrives from the 2nd sm child */
                               				if(record_str[0] == 'T') {

                               					if(h + 1 == 2) {
                               						/* get T */
                               						token = strtok(record_str, delim);
                               						/* get searchers min, max, avg... */
                               						/* min */
                               						token = strtok(NULL, delim);
                               						s_time_min2 = atof(token);
                               						/* max */
                               						token = strtok(NULL, delim);
                               						s_time_max2 = atof(token);
                               						/* avg */
                               						token = strtok(NULL, delim);
                               						s_time_avg2 = atof(token);
									/* sm child execution time*/
                               						token = strtok(NULL, delim);
                               						sm_time2 = atof(token);

                               						break;

                             					}
                            					/* slightly different stats record when h >= 3 */
                               					else {
                               						/* get T */
                               						token = strtok(record_str, delim);
                               						/* get searchers min, max, avg... */
                               						/* min */
                               						token = strtok(NULL, delim);
                               						s_time_min2 = atof(token);
                               						/* max */
                               						token = strtok(NULL, delim);
                               						s_time_max2 = atof(token);
                               		 		       		/* avg */
                               		 		       		token = strtok(NULL, delim);
                               		 		       		s_time_avg2 = atof(token);
                               		 		       		/* get splitter/mergers min,max,avg */
									/* min */
                               		 		       		token = strtok(NULL, delim);
                               		 		       		sm_time_min2 = atof(token);
                               		 		       		/* max */
                               		 		       		token = strtok(NULL, delim);
                               		 		       		sm_time_max2 = atof(token);
                               		 		       		/* avg */
                               		 		       		token = strtok(NULL, delim);
                               		 		       		sm_time_avg2 = atof(token);
                               		 		       		/* sm child execution time*/
                               		 		       		token = strtok(NULL, delim);
                               		 		       		sm_time2 = atof(token);
									
                               		 		       		break;

                            		 		       	}
                   					}		

							if( (nwrite = write(fd_parent, record_str, sizeof(record_str))) < 0) {
								perror("sm:error while writing to parent's pipe");
								exit(1);
							}
						}

						wait(&status);	/* wait for 2nd sm*/

						/* free the new_argv arrays created for passing the new arguments to the sm children */
						for(i = 0; i < 13; ++i) {
							free(new_argv1[i]);
							free(new_argv2[i]);
						}
						free(new_argv1);
						free(new_argv2);

						/* calculate new min,max,avg for the s children */
						if(s_time_min1 < s_time_min2)
							;//s_time_min1 = s_time_min1;

                        			else
                            				s_time_min1 = s_time_min2;

                        			if(s_time_max1 > s_time_max2)
							;//s_time_max1 = s_time_max1;

                        			else
                            				s_time_max1 = s_time_max2;

                        			s_time_avg1 = (s_time_avg1 + s_time_avg2) / 2;

                        			/* calculate min,max,avg for the sm children */
                        			if(h + 1 == 2) {
							if(sm_time1 < sm_time2)
                        			   	     sm_time_min1 = sm_time1;
                        			   	else
                        			   	     sm_time_min1 = sm_time2;

                        			   	if(sm_time1 > sm_time2)
                        			   	     sm_time_max1 = sm_time1;
                        			   	else
                        			   	     sm_time_max1 = sm_time2;

                        			   	sm_time_avg1 = (sm_time1 + sm_time2) / 2;

                        			   	/* stop timer */
                        			   	gettimeofday(&t_end, NULL);

                        			   	/* calculate splitter/merger execution time */
                        			   	time_taken = (t_end.tv_sec - t_start.tv_sec) * 1e6;
                        			   	time_taken = (time_taken + (t_end.tv_usec - t_start.tv_usec)) * 1e-6;

                        			   	/* convert the stats record to string */
                        			   	sprintf(stat_str2, "T %f %f %f %f %f %f %f" , s_time_min1, s_time_max1, s_time_avg1, sm_time_min1, \
													sm_time_max1, sm_time_avg1, time_taken);

                        			   	/* now send the stats record to your parent via named pipe */
                        			   	if( (nwrite = write(fd_parent, stat_str2, sizeof(stat_str2))) < 0) {
                        			   	    perror("sm:error writing stats record to pipe(h=2)");
                        			   	    exit(2);
                        			   	 }
						}
                        			else {

                                			if(sm_time_min1 < sm_time_min2)
								;//sm_time_min1 = sm_time_min1;

                                			else
                                			    sm_time_min1 = sm_time_min2;

                                			if(sm_time_max1 > sm_time_max2)
								;//sm_time_max1 = sm_time_max1;

                                			else
                                			    sm_time_max1 = sm_time_max2;

                                			sm_time_avg1 = (sm_time_avg1 + sm_time_avg2) / 2;

                                			/****************** new sm's ***********************/
                                			if(sm_time1 < sm_time2)
                                			    sm_time_min2 = sm_time1;
                                			else
                                			    sm_time_min2 = sm_time2;

                                			if(sm_time1 > sm_time2)
                                			    sm_time_max2 = sm_time1;
                                			else
                                			    sm_time_max2 = sm_time2;

                                			sm_time_avg2 = (sm_time1 + sm_time2) / 2;

                                			/****************** compare and merge *************/
                                			 if(sm_time_min1 < sm_time_min2)
								 ;//sm_time_min1 = sm_time_min1;

                                			else
                                			    sm_time_min1 = sm_time_min2;

                                			if(sm_time_max1 > sm_time_max2)
								;//sm_time_max1 = sm_time_max1;

                                			else
                                			    sm_time_max1 = sm_time_max2;

                                			sm_time_avg1 = (sm_time_avg1 + sm_time_avg2) / 2;

                                			/* stop timer */
                                			gettimeofday(&t_end, NULL);

                                			/* calculate splitter/merger execution time */
                                			time_taken = (t_end.tv_sec - t_start.tv_sec) * 1e6;
                                			time_taken = (time_taken + (t_end.tv_usec - t_start.tv_usec)) * 1e-6;

                                			/* convert the stats record to string */
                                			sprintf(stat_str2, "T %f %f %f %f %f %f %f" , s_time_min1, s_time_max1, s_time_avg1, sm_time_min1, \
													sm_time_max1, sm_time_avg1, time_taken);

                                			/* now send the stats record to your parent via named pipe */
                                			if( (nwrite = write(fd_parent, stat_str2, sizeof(stat_str2))) < 0) {
                                			    perror("sm:error writing stats record to pipe(h>=3)");
                                			    exit(2);
                                			}
                       				}

						unlink(fifo1);
						unlink(fifo2);

						exit(EXIT_SUCCESS);
				}
		}
	}

	else {		/* H = 1: create 2 s children that will in turn run the searcher executable */

		/* create 2 named pipes for communication between sm process and its children */
		sprintf(fifo1, "fifo_sm_%d_1" , getpid() );
		if(mkfifo(fifo1, 0666) == -1) {
			perror("splitter/merger: mkfifo");
			exit(2);
		}

		sprintf(fifo2, "fifo_sm_%d_2", getpid() );
		if(mkfifo(fifo2, 0666) == -1) {
			perror("splitter/merger: mkfifo");
			exit(2);
		}

		strcpy(str_start1, argv[9]);
		strcpy(str_start2, argv[10]);


		/* array of arguments for the 1st searcher child */
		char **new_argv3 = create_sargs(argv, str_start1, fifo1);

		/* array of arguments for the 2nd searcher child */
		char **new_argv4 = create_sargs(argv, str_start2, fifo2);

		
		pid3 = fork();		/* 1st searcher child */
		switch (pid3) {

			case -1: perror("fork");
				 exit(-1);

			case 0: execvp("./searcher", new_argv3); /* inside 1st seacher child */

			default:	/* inside sm parent after the first searcher child is created */

				/* now open the named pipe that was provided to this sm process as a command line argument by its parent 
				 * sm parent process only writes to this named pipe 
				 */

				if( (fd_parent = open(argv[11], O_WRONLY)) < 0) {
					perror("sm parent fifo open problem");
					exit(-1);
				}

				/* open the named pipe that the 1st searcher child will use to write the results of its search.
				 * Sm parent process only reads from this named pipe 
				 */
					
				if( (fd_child1 = open(fifo1, O_RDONLY)) < 0) {
					perror("searcher child 1 fifo open problem");
					exit(-2);
				}


				pid4 = fork();	/* 2nd searcher child */
				switch (pid4) {

					case -1: perror("fork");
						 exit(-1);

					case 0:  execvp("./searcher", new_argv4); /* inside 2nd searcher child */

					default:
						 /* and then open the named pipe that the 2nd child will use to write the results of its search.
						  * Sm parent process only reads from this named pipe
						  */
						
						 if( (fd_child2 = open(fifo2, O_RDONLY)) < 0) {
							perror("s child 2 fifo open problem");
							exit(-3);
						}

						/* while there are bytes to read from searcher child 1's pipe, write these bytes to your parent's named pipe */
						while( (nread = read(fd_child1, record_str, sizeof(record_str))) > 0 ) {

							/* the searcher child sent its execution time */
							if(record_str[0] == 'T') {
								/* get T */
								token = strtok(record_str, delim);
								/* get the time */
								token = strtok(NULL, delim);
								child1_time = atof(token);

								/* after that break the while loop */

								break;
							}

							if(  (nwrite = write(fd_parent, record_str, sizeof(record_str)))  < 0) { 
								perror("sm:error while writing to parent's pipe");
								exit(1);
							}
						}

						wait(&status);	/* wait 1st s child */

						/* while there are bytes to read from searcher child 2's pipe, write these bytes to your parent's named pipe */
						while( (nread = read(fd_child2, record_str, sizeof(record_str))) > 0 ) {

							/* the searcher child sent its execution time */
							if(record_str[0] == 'T') {
								/* get T */
								token = strtok(record_str, delim);
								/* get the time */
								token = strtok(NULL, delim);
								child2_time = atof(token);

								/* after that break the while loop */

								break;
							}

							if( (nwrite = write(fd_parent, record_str, sizeof(record_str))) < 0) {
								perror("sm:error while writing to parent's pipe");
								exit(1);
							}
						}

						wait(&status);	/* wait 2nd searcher child to finish */


						/* at this point the sm process has gathered both the execution times of its searcher children 
						 * calculate min,max,avg of its children's execution times 
						 */

						double min, max, avg;

						if(child1_time < child2_time) {
							min = child1_time;
							max = child2_time;
						}
						else {
							min = child2_time;
							max = child1_time;
						}
						avg = (min + max) / 2;

						/* free the argument arrays created dynamically */
						for(i = 0; i < 10; ++i) {
							free(new_argv3[i]);
							free(new_argv4[i]);
						}
						free(new_argv3);
						free(new_argv4);


						/* stop timer */
						gettimeofday(&t_end, NULL);

						/* calculate splitter/merger execution time */
						time_taken = (t_end.tv_sec - t_start.tv_sec) * 1e6;
						time_taken = (time_taken + (t_end.tv_usec - t_start.tv_usec)) * 1e-6;

						/* now create a record of the time statistics that the sm process has gathered 
						 * and send them to its sm parent (records of time stats start with a 'T')
						 */

						sprintf(stat_str1, "T %f %f %f %f" , min, max, avg, time_taken);

						if( (nwrite = write(fd_parent, stat_str1, 38)) < 0) {
							perror("sm:error while writing sm stats to parent's pipe");
							exit(1);
						}

						unlink(fifo1);
						unlink(fifo2);

						exit(EXIT_SUCCESS);
				}
		}
	}
}
