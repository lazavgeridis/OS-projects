/* sm_utils.c */

#include <stdlib.h>
#include <string.h>

#define SMARGS 14	/* number of splitter/merger arguments */
#define SARGS  11	/* number of searcher arguments */

char **create_smargs(char **argv, char *height, char *start_range, char *end_range, char *fifo_name)
{
	char **new_argv;

	new_argv = malloc( SMARGS * sizeof(char *) );
	/* copy "-h" */
	new_argv[0] = malloc(strlen(argv[0]) + 1);
	strcpy(new_argv[0], argv[0]);
	/* copy updated height */
	new_argv[1] = malloc(strlen(height) + 1);
	strcpy(new_argv[1], height);
	/* now copy to new_argv up to "searchers"(argv[8]) from argv. These strings remain unchanged */
	for(int i = 2; i < 9; ++i) {
		new_argv[i] = malloc(strlen(argv[i]) + 1);
		strcpy(new_argv[i], argv[i]);
	}
	/* copy to the 1st sm child start1 and end1 */
	new_argv[9] = malloc(strlen(start_range) + 1);
	strcpy(new_argv[9], start_range);
	new_argv[10] = malloc(strlen(end_range) + 1);
	strcpy(new_argv[10], end_range);
	/* copy the new named pipe file(fifo1) */
	new_argv[11] = malloc(strlen(fifo_name) + 1);
	strcpy(new_argv[11], fifo_name);
	/* copy root's pid */
	new_argv[12] = malloc(strlen(argv[12]) + 1);
	strcpy(new_argv[12], argv[12]);
	/* append NULL */
	new_argv[13] = NULL;

	return new_argv;
}

char **create_sargs(char **argv, char *start_range, char *fifo_name)
{
	char **new_argv;

	new_argv = malloc( SARGS * sizeof(char *) );
	for(int i = 2; i < 9; ++i) {
		new_argv[i - 2] = malloc(strlen(argv[i]) + 1);
		strcpy(new_argv[i - 2], argv[i]);
	}
	/* searcher number */
	new_argv[7] = malloc(strlen(start_range) + 1);
	strcpy(new_argv[7], start_range);
	/* pipe name */
	new_argv[8] = malloc(strlen(fifo_name) + 1);
	strcpy(new_argv[8], fifo_name);
	/* copy root pid */
	new_argv[9] = malloc(strlen(argv[12]) + 1);
	strcpy(new_argv[9], argv[12]);
	/* append NULL */
	new_argv[10] = NULL;

	return new_argv;
}
