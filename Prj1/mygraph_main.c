#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "PromptInterface.h"
#include "Utilities.h"


int main(int argc, char *argv[])
{
	char *buffer = NULL, *token = NULL, name1[25], name2[25]; 
	const char delim[3] = " \t";
	int i, weight;
	size_t	bufsize;
	ssize_t linelen;

	/* allocate memory for the graph and initialize it */
	Graph *G = (Graph *)malloc( sizeof(Graph) );
	if(G == NULL) {					
		printf("Insufficient memory!\n");
		return -1;
	}
	InitializeGraph(&G);

	/* initialize the global adj.list head pointers */
	initStack();
	initCycle();
	initBlockedSet();

	/*  $ ./myprog  */
	if(argc == 1) {

		/* neither flag was given */
		/* we don't take any input  and we don't write the output to a file(final state of the graph) */
		/* initiate the interface */

		while( (linelen = getline(&buffer, &bufsize, stdin) ) > 0 ) {	/* while input is given and we haven't reached EOF(getline returns -1 on failure) */

			token = strtok(buffer, delim);	
			/* based on the first letter of the string given we can define which command needs to be executed */
			switch(buffer[0]) {

				case 'i':	token = strtok(NULL, delim);	
						remove_newline(token);
						strcpy(name1, token);
						InsertNode(&G, name1);
						break;

				case 'n':	token = strtok(NULL, delim);
						strcpy(name1, token);
						token = strtok(NULL, delim);
						strcpy(name2, token);
						token = strtok(NULL, delim);
						weight = atoi(token);	
						InsertEdge(&G, name1, name2, weight);
						break;

				case 'd':	token = strtok(NULL, delim);
						remove_newline(token);
						strcpy(name1, token);
						DeleteNode(&G, name1);
						break;

				case 'l':	token = strtok(NULL, delim);
						strcpy(name1, token);
						token = strtok(NULL, delim);
						strcpy(name2, token);
						token = strtok(NULL, delim);
						if(token == NULL) {				
							for(i = 0; i < (int)strlen(name2); ++i) {
								if(name2[i] == '\n') { 
									name2[i] = '\0';
									break;
								}
							}

							DeleteAllEdges(&G,name1, name2);
						}
						else {

							weight = atoi(token);
							DeleteEdge(&G, name1, name2, weight);
						}
						break;

				case 'm':	token = strtok(NULL, delim);
						strcpy(name1, token);
						token = strtok(NULL, delim);
						strcpy(name2, token);
						token = strtok(NULL, delim);
						weight = atoi(token);
						token = strtok(NULL, delim);
						int nweight = atoi(token);
						ModifyWeight(&G, name1, name2, weight, nweight);
						break;

				case 'r':	token = strtok(NULL, delim);
						remove_newline(token);
						strcpy(name1, token);
						ReceivingEdges(&G, name1);
						break;

				case 'c':	token = strtok(NULL, delim);
						remove_newline(token);
						strcpy(name1, token);
						CircleFind(&G, name1);
						break;

				case 'f':	token = strtok(NULL, delim);
						strcpy(name1, token);
						token = strtok(NULL, delim);
						weight = atoi(token);
						FindCircles(&G, name1, weight);
						break;


				case 't':	token = strtok(NULL, delim);
						strcpy(name1, token);
						token = strtok(NULL, delim);
						strcpy(name2, token);
						token = strtok(NULL, delim);
						weight = atoi(token);
						Traceflow(&G, name1, name2, weight);
						break;


				case 'e':	/* exit function just to free the allocated memory */
						Exit(&G);
						break;	
			}
			if(buffer[0] == 'e') {
				free(buffer);
				buffer = NULL;
				break;
			}
			else {
				free(buffer);
				buffer = NULL;
			}
		}
		if(buffer) free(buffer); /* free the buffer one last time when getline() fails */
	}

	/* $ ./myprog -i input  OR  $ ./myprog -o output */
	if(argc == 3) {

		FILE *f_handle;
		char filename[50];

		if(strcmp(argv[1], "-i") == 0 ) {

			/* read from input file in argv[2] and create the graph */
			f_handle = fopen(argv[2], "r");
			if(f_handle == NULL) {
				printf("Couldn't open file!\n");
				return -1;
			}
			char line[100];

			while(fgets(line, 100, f_handle) ) { 	/* while we haven't reached end of file */

				token = strtok(line, delim);	/* read the first name until delimeter */
				strcpy(name1, token);
				token = strtok(NULL, delim);	/* read the second name until delimeter */
				strcpy(name2, token);
				token = strtok(NULL, delim);	/* read the edge's weight */
				weight = atoi(token);		/* convert it to integer */
				InsertEdge(&G, name1, name2, weight);	/* insert edge */
			}
			fclose(f_handle);	/* after we get all the information for the graph, close the file */

			/* after the graph is created initiate the user interface */
		}
		else {
			/* argv[1] is "-o" so output file is argv[2] */
			/* no input file in this case */
			/* initiate the command prompt interface directly */
			strcpy(filename, argv[2]);
		}

		while( (linelen = getline(&buffer, &bufsize, stdin) ) > 0 ) {
			token = strtok(buffer, delim);
			switch(buffer[0]) {

				case 'i':	token = strtok(NULL, delim);	
						remove_newline(token);
						strcpy(name1, token);
						InsertNode(&G, name1);
						break;

				case 'n':	token = strtok(NULL, delim);
						strcpy(name1, token);
						token = strtok(NULL, delim);
						strcpy(name2, token);
						token = strtok(NULL, delim);
						weight = atoi(token);	
						InsertEdge(&G, name1, name2, weight);
						break;

				case 'd':	token = strtok(NULL, delim);
						remove_newline(token);
						strcpy(name1, token);
						DeleteNode(&G, name1);
						break;

				case 'l':	token = strtok(NULL, delim);
						strcpy(name1, token);
						token = strtok(NULL, delim);
						strcpy(name2, token);
						token = strtok(NULL, delim);
						if(token == NULL) {				
							for(i = 0; i < (int)strlen(name2); ++i) {
								if(name2[i] == '\n') { 
									name2[i] = '\0';
									break;
								}
							}
							DeleteAllEdges(&G, name1, name2);
						}
						else {

							weight = atoi(token);
							DeleteEdge(&G, name1, name2, weight);
						}
						break;

				case 'm':	token = strtok(NULL, delim);
						strcpy(name1, token);
						token = strtok(NULL, delim);
						strcpy(name2, token);
						token = strtok(NULL, delim);
						weight = atoi(token);
						token = strtok(NULL, delim);
						int nweight = atoi(token);
						ModifyWeight(&G, name1, name2, weight, nweight);
						break;

				case 'r':	token = strtok(NULL, delim);
						remove_newline(token);
						strcpy(name1, token);
						ReceivingEdges(&G, name1);
						break;

				case 'c':	token = strtok(NULL, delim);
						remove_newline(token);
						strcpy(name1, token);
						CircleFind(&G, name1);
						break;

				case 'f':	token = strtok(NULL, delim);
						strcpy(name1, token);
						token = strtok(NULL, delim);
						weight = atoi(token);
						FindCircles(&G, name1, weight);
						break;


				case 't':	token = strtok(NULL, delim);
						strcpy(name1, token);
						token = strtok(NULL, delim);
						strcpy(name2, token);
						token = strtok(NULL, delim);
						weight = atoi(token);
						Traceflow(&G, name1, name2, weight);
						break;


				case 'e':	/* if argv[1] == -i exit function just to free memory */
						/* if argv[1] == -o exit function for both freeing memory and printing the graph to output file */
						if(strcmp(argv[1], "-i") == 0)
							Exit(&G);
						else
							Exit_out(&G, filename);
						break;
			}
			free(buffer);
			buffer = NULL;
		}
		if(buffer) free(buffer);
	}

	if(argc == 5) {

		FILE *f_handle;
		char filename[50];

		/* both -i and -o flags were given */
		/* we need to find out whether the -i flag is argv[1] or argv[3] */
		if(strcmp(argv[1], "-i") == 0) {
			
			/* input file is argv[2] and output file is argv[4] */

			f_handle = fopen(argv[2], "r");
			strcpy(filename, argv[4]);
		}
		else {
			/* input file is argv[4] and output file is argv[2] */

			f_handle = fopen(argv[4], "r");
			strcpy(filename, argv[2]);
		}
		
		if(f_handle == NULL) {
			printf("Couldn't open file!\n");
			return -1;
		}
		/* create the graph using the input file */
		char line[100];
		while( fgets(line, 100, f_handle) ) { 

			token = strtok(line, delim);
			strcpy(name1, token);
			token = strtok(NULL, delim);
			strcpy(name2, token);
			token = strtok(NULL, delim);
			weight = atoi(token);
			InsertEdge(&G, name1, name2, weight);
		}
		fclose(f_handle);	/* we dont need the input file anymore */

		/* initiate the command prompt interface */

		while( (linelen = getline(&buffer, &bufsize, stdin) ) > 0 ) {
				token = strtok(buffer, delim);
				switch(buffer[0]) {

					case 'i':	token = strtok(NULL, delim);	
							remove_newline(token);
							strcpy(name1, token);
							InsertNode(&G, name1);
							break;

					case 'n':	token = strtok(NULL, delim);
							strcpy(name1, token);
							token = strtok(NULL, delim);
							strcpy(name2, token);
							token = strtok(NULL, delim);
							weight = atoi(token);	
							InsertEdge(&G, name1, name2, weight);
							break;

					case 'd':	token = strtok(NULL, delim);
							remove_newline(token);
							strcpy(name1, token);
							DeleteNode(&G, name1);
							break;

					case 'l':	token = strtok(NULL, delim);
							strcpy(name1, token);
							token = strtok(NULL, delim);
							strcpy(name2, token);
							token = strtok(NULL, delim);
							if(token == NULL) {				/* no weight was given */
								for(i = 0; i < (int)strlen(name2); ++i) {
									if(name2[i] == '\n') { 
										name2[i] = '\0';
										break;
									}
								}
								DeleteAllEdges(&G, name1, name2);
							}
							else {

								weight = atoi(token);
								DeleteEdge(&G, name1, name2, weight);
							}
							break;

					case 'm':	token = strtok(NULL, delim);
							strcpy(name1, token);
							token = strtok(NULL, delim);
							strcpy(name2, token);
							token = strtok(NULL, delim);
							weight = atoi(token);
							token = strtok(NULL, delim);
							int nweight = atoi(token);
							ModifyWeight(&G, name1, name2, weight, nweight);
							break;

					case 'r':	token = strtok(NULL, delim);
							remove_newline(token);
							strcpy(name1, token);
							ReceivingEdges(&G, name1);
							break;

					case 'c':	token = strtok(NULL, delim);
							remove_newline(token);
							strcpy(name1, token);
							CircleFind(&G, name1);
							break;

					case 'f':	token = strtok(NULL, delim);
							strcpy(name1, token);
							token = strtok(NULL, delim);
							weight = atoi(token);
							FindCircles(&G, name1, weight);
							break;


					case 't':	token = strtok(NULL, delim);
							strcpy(name1, token);
							token = strtok(NULL, delim);
							strcpy(name2, token);
							token = strtok(NULL, delim);
							weight = atoi(token);
							Traceflow(&G, name1, name2, weight);
							break;


					case 'e':	Exit_out(&G, filename);
							break;
				
				}
				free(buffer);
				buffer = NULL;
		}
		if(buffer) free(buffer);
	}

	return 0;
}
