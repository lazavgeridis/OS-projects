/* directory_tree.c */

#include <unistd.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#include "DirectoryTreeInterface.h"
#include "inotify_events.h"

volatile sig_atomic_t exit_signal = 0;

void sig_handler(int signum) { exit_signal = 1; }
void remove_directory(node **, inode_entry **, char *);

int main(int argc, char *argv[]) {

	DIR *dir_ptr1, *dir_ptr2;
	char *path_source, *path_backup;
	struct stat info1, info2;
	struct sigaction sig;
	inode_entry *inode_table1 = NULL, *inode_table2 = NULL;
	node *root_source, *root_backup;


	if(argc != 3) { 
		fprintf(stderr, "\nProvide both directories!\nUsage:\n\t./mirr <source_directory> <backup_directory>\n\n"); 
		exit(EXIT_FAILURE);
	}


	/* function to be executed when a SIGINT signal is caught is sig_handler */
	memset(&sig, '\0', sizeof sig);
	sig.sa_handler = sig_handler;
	/* block every other signal when running sig_handler */
	sigfillset(&sig.sa_mask);
	if(sigaction(SIGINT, &sig, NULL) == -1) {
		perror("sigaction");
		exit(-1);
	}

	if( (dir_ptr1 = opendir(argv[1])) == NULL) { 
		perror("cannot open source directory!\n");
		exit(EXIT_FAILURE);
	}

	if( (dir_ptr2 = opendir(argv[2])) == NULL) {
		perror("cannot open backup directory!\n");
		exit(EXIT_FAILURE);
	}

	if(stat(argv[1], &info1) == -1) {
		perror("stat'ing the source directory!\n");
		exit(EXIT_FAILURE);
	}

	if(stat(argv[2], &info2) == -1) {
		perror("stat'ing the backup directory!\n");
		exit(EXIT_FAILURE);
	}


	/* construct Source directory tree */
	root_source = create_node(argv[1], info1.st_ino);
	construct_tree(&inode_table1, argv[1], &root_source);

	/* construct Backup directory tree */
	root_backup = create_node(argv[2], info2.st_ino);
	construct_tree(&inode_table2, argv[2], &root_backup);

	/* sort siblings on each level of each tree */
	sort_siblings(&root_source);
	sort_siblings(&root_backup);


	/******************* Synchronization Steps *********************/

	path_source = (char *)malloc(strlen(root_source->name) + 1);    /* +1 for the '\0' */
	if(path_source == NULL) {
		fprintf(stderr, "Insufficient memory!\n");
		exit(EXIT_FAILURE);
	}
	strcpy(path_source, root_source->name);

	path_backup = (char *)malloc(strlen(root_backup->name) + 1);    /* +1 for the '\0' */
	if(path_backup == NULL) {
		fprintf(stderr, "Insufficient memory!\n");
		exit(EXIT_FAILURE);
	}
	strcpy(path_backup, root_backup->name);

	/* sync the 2 directories - Source and Backup - */
	sync_directories(&root_source, &root_backup, &inode_table1, &inode_table2, path_source, path_backup);



	printf("\n\n======================== Sync Step (a) - Done -  =============================\n\n");

	free(path_source);
	path_source = NULL;
	
	printf("======================== Sync step!(b) ==============================\n\n");


	/* initialize an inotify instance */
	int inotify_fd = inotify_init();
	if(inotify_fd == -1) {
		perror("inotify_init");
		exit(EXIT_FAILURE);
	}


	/* Now, start the 2nd sync step. 
     * Add watch descriptors recursively to each directory in Source 
     */
	watch_descriptor *wd_listhead = NULL;
	add_watch_descriptors(&wd_listhead, inotify_fd, root_source, path_source, root_source->name); 

    printf("\n\nAdded watch descriptors on each directory under Source\n\n");

	/* then, start monitoring inotify events */
	int length, read_ptr, read_offset = 0; /* remaining number of bytes from previous read */
	int wd, modified = 0, cookie = 0, moved_from = 0, flag = 0, no_create = 0, from_create = 0, oldinode;
	char buffer[EVENT_BUF_LEN], move_name[35], *modified_name = NULL;
	const char delim[2] = ".";

	while(!exit_signal) {

		/* read next series of events */
		length = read(inotify_fd, buffer + read_offset, sizeof(buffer) - read_offset);
		if (length < 0) perror("read");
		if(errno == EINTR) break;
		length += read_offset; /* if there was an offset, add it to the number of bytes to process */
		read_ptr = 0;
		
		/* process each event
         * make sure at least the fixed part of the event in included in the buffer
         */
		while (read_ptr + EVENT_SIZE <= length ) { 
			//point event to beginning of fixed part of next inotify_event structure
			struct inotify_event *event = (struct inotify_event *) &buffer[ read_ptr ];
			
			// if however the dynamic part exceeds the buffer, 
			// that means that we cannot fully read all event data and we need to 
			// deffer processing until next read completes
			if( read_ptr + EVENT_SIZE + event->len > length ) 
				break;

			//event is fully received, process
			printf("WD:%i %s %s %s COOKIE=%u\n", 
			event->wd, event_name(event), 
			target_type(event), target_name(event), event->cookie);
			

			/* ======================== examine type of event ============================ */


			if( (moved_from) && ( !(event->mask & IN_MOVED_TO) ) ) { /* previous event was in_moved_from, but this event is not in_moved_to */

				moved_from = 0;
				cookie = 0;

				/* The filename that caused the IN_MOVED_FROM event still has a copy in Backup.
                 * Unlink the filename from Backup. 
                 */
				mvfrom_unlink(wd_listhead, root_source->name, root_backup->name, wd, move_name);
				wd = 0; 
			}


			if(event->mask & IN_CREATE) {
				if( (event->name[0] != '.') && (event->name[0] != '4') && (!no_create) && (event->name[strlen(event->name) - 1] != '~') ) {
								
					printf("\n----- IN_CREATE Event! -----\n");
					in_create(event, inotify_fd, &wd_listhead, &root_source, &inode_table1, &root_backup, &inode_table2);
					from_create = 1;
                    if( (event->mask & IN_ISDIR) == 0)
                        flag = 1;
				}
				no_create = 0;
			}

			else if(event->mask & IN_ATTRIB) {
				if( (event->name[0] != '.') && (event->name[0] != '4') && (!from_create) && (event->name[strlen(event->name) - 1] != '~') ) {
					printf("\n----- IN_ATTRIB Event! -----\n");
					in_attrib(event, wd_listhead, &root_source, &inode_table1, &root_backup, &inode_table2, oldinode); /* ++++++++++++++ */
                    //flag = 1;
				}
				//from_create = 0;
			}

			else if(event->mask & IN_MODIFY) {
				if( (event->name[0] != '.') && (event->name[0] != '4') && (!from_create) && (event->name[strlen(event->name) - 1] != '~') ) {
					printf("\n----- IN_MODIFY Event! -----\n");
					modified = in_modify(event);	/* if it is a file, mark it as modified */
				}
				/* when a file is modified it is renamed to .filename.swp (at least when editing the file in vim) */
				if(event->name[0] == '.') {
					if(modified_name != NULL) {
						free(modified_name);
						modified_name = NULL;
					}
					char *token = strtok(&event->name[1], delim);   /* acquire the original filename (use strtok on the string .filename.swp) */
					modified_name = (char *)malloc(strlen(token) + 1);
					strcpy(modified_name, token);
                    
                    /* get the i-node of the file that is currently being modified, since after the modification, the i-node changes */
					if( ( oldinode = get_inode(event, wd_listhead, root_source->name, modified_name) ) != -1 ) {
                        if(flag == 1) flag = 2;
					    printf("Modifying file %s (i-node %d)\n", modified_name, oldinode);
                    }
				}
			}
            
			else if(event->mask & IN_CLOSE_WRITE) {
				if( (event->name[0] != '.') && (event->name[0] != '4') && (!from_create) && (event->name[strlen(event->name) - 1] != '~') ) {
					printf("\n----- IN_CLOSE_WRITE Event! -----\n");
					if(modified == 1) {
						oldinode = in_close_write(event, wd_listhead, &root_source, &inode_table1, &root_backup, &inode_table2);
						modified = 0;
					}
				}
				from_create = 0;
			}

			else if(event->mask & IN_DELETE) {
				if( (event->name[0] != '.') && (event->name[0] != '4') && (event->name[strlen(event->name) - 1] != '~') ) {
					printf("\n----- IN_DELETE Event! -----\n");
					in_delete(event, wd_listhead, &root_source, &inode_table1, &root_backup, &inode_table2);
				}
			}

			else if(event->mask & IN_DELETE_SELF) {
				if( (event->name[0] != '.') && (event->name[0] != '4') && (event->name[strlen(event->name) - 1] != '~') ) {
					printf("\n----- IN_DELETE_SELF Event! -----\n");
					in_delete_self(event, inotify_fd, wd_listhead, &root_source, &inode_table1, &root_backup, &inode_table2);
				}
			}

			else if(event->mask & IN_MOVED_FROM) {
				if( (event->name[0] != '.') && (event->name[0] != '4') && (event->name[strlen(event->name) - 1] != '~') ) {
					//		flag = 1;
					//		no_create = 1;
					printf("\n----- IN_MOVED_FROM Event! -----\n");
					if(flag != 2) {
						/********** in_delete again ************/
						in_delete(event, wd_listhead, &root_source, &inode_table1, &root_backup, &inode_table2);
						moved_from = 1;
						cookie = event->cookie;
						strcpy(move_name, event->name);
						wd = event->wd;
					}
					flag = 0;
                    no_create = 1;
				}
			}

			if(event->mask & IN_MOVED_TO) {
				if( (event->name[0] != '.') && (event->name[0] != '4') && (!no_create) && (event->name[strlen(event->name) - 1] != '~') ) {
					if(event->cookie == (unsigned int)cookie) {
						printf("\n----- IN_MOVED_TO Event! -----\n");
						in_moved_to(wd, move_name, event, wd_listhead, &root_source, &inode_table1, &root_backup, &inode_table2);
						moved_from = 0;
						cookie = 0;
						wd = 0;
					}
					else {
						/* act as in_create */
						in_create(event, inotify_fd, &wd_listhead, &root_source, &inode_table1, &root_backup, &inode_table2);
						moved_from = 0;
						cookie = 0;
						wd = 0;
					}
				}
			}


			//advance read_ptr to the beginning of the next event
			read_ptr += EVENT_SIZE + event->len;
		}

		//check to see if a partial event remains at the end
		if( read_ptr < length ) {
			//copy the remaining bytes from the end of the buffer to the beginning of it
			memcpy(buffer, buffer + read_ptr, length - read_ptr);
			//and signal the next read to begin immediatelly after them			
			read_offset = length - read_ptr;
		} else
			read_offset = 0;

	    free(modified_name);
        modified_name = NULL;
	}

    if(modified_name) 
        free(modified_name);

    cleanup(&root_source, &root_backup, &inode_table1, &inode_table2, &wd_listhead, inotify_fd);
	close(inotify_fd);
	if(closedir(dir_ptr1) < 0) perror("closedir");
	if(closedir(dir_ptr2) < 0) perror("closedir");
	free(path_source);
	free(path_backup);

	exit(EXIT_SUCCESS);
}
