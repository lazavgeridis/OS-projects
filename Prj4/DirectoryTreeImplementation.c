/* DirectoryTreeImplementation.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>

#include "DirectoryTreeUtilities.h"


node *create_node(char *name, int inode_num) { 

	node *newnode = (node *)malloc( sizeof(node) );
	if(!newnode) {
		fprintf(stderr, "Insufficient memory!\n");
		exit(EXIT_FAILURE);
	}
	strcpy(newnode->name, name);
	newnode->inode = inode_num;
	newnode->dir_children = 0;
	newnode->file_children = 0;
	newnode->list_of_dir_children = NULL;
	newnode->list_of_file_children = NULL;
	newnode->prev_dir_sibling = NULL;
	newnode->next_dir_sibling = NULL;
	newnode->prev_file_sibling = NULL;
	newnode->next_file_sibling = NULL;

	return newnode;
}


void construct_tree(inode_entry **inotable, char *dirpath, node **cur_dirnode) {

	DIR *dirptr;
	struct dirent *dir;
	struct stat info;
	char *buffer;
	node *pcrawl1 = *cur_dirnode;
	node *pcrawl2 = *cur_dirnode;

	if( ( dirptr = opendir(dirpath) ) == NULL) perror("cannot open directory");

	/* loop to acquire info about every file/directory in the current directory */
	while( ( dir = readdir(dirptr) ) != NULL ) {

		/* skip current directory, parent directory, and entities that have been deleted */
		if( (strcmp(dir->d_name, ".") == 0) || (strcmp(dir->d_name, "..") == 0) || (dir->d_ino == 0) ) continue;

		/* create the path and store it in buffer */
		buffer = create_path2(dirpath, dir->d_name);
		/* get info about this entity with stat() */
		if( stat(buffer, &info) == -1) perror(buffer);

		/* determine whether the entity is a file or a directory */
		if( (info.st_mode & S_IFMT) == S_IFDIR) { /* entity is a Directory */

			insert_entry(inotable, buffer, info.st_ino, ctime(&info.st_mtime), info.st_size);  /* i-node entry corresponding to this directory */

			/* add a directory treenode to the current tree and then recursively explore the contents of this directory */
			if( (*cur_dirnode)->dir_children == 0) {    /* Current directory treenode has no directory children */
				pcrawl1->list_of_dir_children = create_node(dir->d_name, info.st_ino);
				(*cur_dirnode)->dir_children++;
				pcrawl1 = pcrawl1->list_of_dir_children;
				construct_tree(inotable, buffer, &pcrawl1);
			}
			else {  /* Current directory treenode has directory children */
                node *new_dirnode =  create_node(dir->d_name, info.st_ino);
				pcrawl1->next_dir_sibling = new_dirnode;
                new_dirnode->prev_dir_sibling = pcrawl1; 
				(*cur_dirnode)->dir_children++;
				pcrawl1 = pcrawl1->next_dir_sibling;
				construct_tree(inotable, buffer, &pcrawl1);
			}
		}
		else {  /* entity is a File */

			insert_entry(inotable, buffer, info.st_ino, ctime(&info.st_mtime), info.st_size);  /* i-node entry corresponding to this file */

			/* add a file treenode to the current tree */
			if( (*cur_dirnode)->file_children == 0) {
				pcrawl2->list_of_file_children = create_node(dir->d_name, info.st_ino);
				(*cur_dirnode)->file_children++;
				pcrawl2 = pcrawl2->list_of_file_children;
			}
			else {
                node *new_filenode = create_node(dir->d_name, info.st_ino);
				pcrawl2->next_file_sibling = new_filenode;
                new_filenode->prev_file_sibling = pcrawl2;
				(*cur_dirnode)->file_children++;
				pcrawl2 = pcrawl2->next_file_sibling;
			}
		}

		free(buffer);
		buffer = NULL;
	}

	if(closedir(dirptr) < 0) perror("closedir");
}


void sort_siblings(node **dir) {

	char buf[40];
	int i, temp_inode, temp_dir_children, temp_file_children;
	node *temp, *temp_next, *temp_dir_list, *temp_file_list;

	if( (*dir)->dir_children > 0 ) {

		temp = (*dir)->list_of_dir_children;
		temp_next = (*dir)->list_of_dir_children->next_dir_sibling;
		/* loop to sort directory children of dir */
		while(temp_next != NULL) {

			while(temp_next != temp) {
				/* not sorted alphabetically, need to swap all the values */
				if( strcmp(temp->name, temp_next->name) > 0) {

					/* move all the data fields of temp to temp variables */
					strcpy(buf, temp->name);
					temp_inode = temp->inode;
					temp_dir_children = temp->dir_children;
					temp_file_children = temp->file_children;
					temp_dir_list = temp->list_of_dir_children;
					temp_file_list = temp->list_of_file_children;
					/* now copy the data fields of temp_next to temp */
					strcpy(temp->name, temp_next->name);
					temp->inode = temp_next->inode;
					temp->dir_children = temp_next->dir_children;
					temp->file_children = temp_next->file_children;
					temp->list_of_dir_children = temp_next->list_of_dir_children;
					temp->list_of_file_children = temp_next->list_of_file_children;
					/* lastly, copy the data fields of temp, that we previously stored in temp variables, to temp_next */
					strcpy(temp_next->name, buf);
					temp_next->inode = temp_inode;
					temp_next->dir_children = temp_dir_children;
					temp_next->file_children = temp_file_children;
					temp_next->list_of_dir_children = temp_dir_list;
					temp_next->list_of_file_children = temp_file_list;
				}
				temp = temp->next_dir_sibling;
			}
			temp = (*dir)->list_of_dir_children;
			temp_next = temp_next->next_dir_sibling;
		}
	}

	if( (*dir)->file_children > 0 ) {

		/* loop to sort file children of dir */
		temp = (*dir)->list_of_file_children;
		temp_next = (*dir)->list_of_file_children->next_file_sibling;
		while(temp_next != NULL) {

			while(temp_next != temp) {
				/* not sorted alphabetically, need to swap all the values */
				if( strcmp(temp->name, temp_next->name) > 0) {

					/* move all the data fields of temp to temp variables */
					strcpy(buf, temp->name);
					temp_inode = temp->inode;
					/* now copy the data fields of temp_next to temp */
					strcpy(temp->name, temp_next->name);
					temp->inode = temp_next->inode;
					/* lastly, copy the data fields of temp, that we previously stored in temp variables, to temp_next */
					strcpy(temp_next->name, buf);
					temp_next->inode = temp_inode;
				}
				temp = temp->next_file_sibling;
			}
			temp = (*dir)->list_of_file_children;
			temp_next = temp_next->next_file_sibling;
		}
	}

	/* sort all directory nodes in the tree recursively */
	temp = (*dir)->list_of_dir_children;
	for(i = 0; i < (*dir)->dir_children; i++) {
		sort_siblings(&temp);
		temp = temp->next_dir_sibling;
	}
}


void sync_directories(node **dir_s, node **dir_b, inode_entry **inotable_s, inode_entry **inotable_b, char *path_s, char *path_b) {

	struct stat info;
	char *path_source, *path_backup, lastmod[29];
	int size, flag = 0;

	node *prev, *next, *newnode;
	node *pcrawl1 = NULL; 
	node *pcrawl2 = NULL;

	printf("\nSource->%s with %d dir children and %d file children\n", (*dir_s)->name, (*dir_s)->dir_children, (*dir_s)->file_children);
	printf("Backup->%s with %d dir children and %d file children\n", (*dir_b)->name, (*dir_b)->dir_children, (*dir_b)->file_children);


/********************************************** DIRECTORIES ************************************************************/

	/* firstly, sync the Directory entities in the current tree level */

	if( (*dir_s)->dir_children > 0) {

		pcrawl1 = (*dir_s)->list_of_dir_children;
        pcrawl2 = (*dir_b)->list_of_dir_children;

		while(pcrawl1 != NULL) {

			/* create the paths for source and backup */
			path_source = create_path2(path_s, pcrawl1->name);
			path_backup = create_path2(path_b, pcrawl1->name);
			printf("Exists: %s\n", path_source);
			printf("Exists(?): %s\n", path_backup);


			/* check if the directory node in Backup has any directory node children */
			if( (*dir_b)->dir_children > 0 && !flag ) { 

				/* Directory name also exists in Backup */
				if(strcmp(pcrawl1->name, pcrawl2->name) == 0) {

					printf("Case 1) Directory names matched: %s\n", pcrawl2->name);

					link_source_backup(inotable_s, pcrawl1->inode, pcrawl2->inode);

                    /* sync the directory level "under" the current one */
					sync_directories(&pcrawl1, &pcrawl2, inotable_s, inotable_b, path_source, path_backup);


					/* don't add directory treenode to backup tree + don't add i-node entry for dir in the backup i-node table */

                    pcrawl1 = pcrawl1->next_dir_sibling;

                    if(!pcrawl2->next_dir_sibling && pcrawl1)
                        flag = 1;
                    else
                        pcrawl2 = pcrawl2->next_dir_sibling;

				}
				else {

					if(strcmp(pcrawl1->name, pcrawl2->name) > 0) {
						/* directory folder in backup should not exist in backup - delete it and all of its contents */

						printf("Case 2)Directory folder %s must be removed from backup!(%s > %s)\n", pcrawl2->name, pcrawl1->name, pcrawl2->name);

						free(path_backup); 
						path_backup = NULL;
						path_backup = create_path2(path_b, pcrawl2->name);

						printf("Before remove_directory: %s has %d directory children\n", (*dir_b)->name, (*dir_b)->dir_children);

						/* function to delete the whole directory */
						remove_directory(&pcrawl2, inotable_b, path_backup, 0);

						printf("After remove_directory\n");

                        prev = pcrawl2->prev_dir_sibling; /* ++++++++++++++++++++++ */
                        next = pcrawl2->next_dir_sibling; /* ++++++++++++++++++++++ */

						delete_node(dir_b, pcrawl2->name, 'd');    /* delete a directory treenode */

						if(rmdir(path_backup) < 0) perror("rmdir");

						/* don't add directory treenode to backup tree + don't add i-node entry in the backup i-node table */
						
						printf("%s now has %d dir children\n", (*dir_b)->name, (*dir_b)->dir_children);

                        if(!next) {
                            flag = 1;
                            pcrawl2 = prev;
                        }
                        else
                            pcrawl2 = next;

					}
					else {

						/* directory folder in backup with name "cur->name" may exist in source, just later in the directory list */
						/* nevertheless, a new directory treenode must be added in Backup */

						printf("Case 3) Directory folder in Backup might exist later in Source! (%s < %s)\n", pcrawl1->name, pcrawl2->name);

				        /* - Check if a file node exists in Backup with the same name as the directory node in Source -
                         * - If it does, we need to remove the file entity from the system; we cannot have both a directory and a file
                         *   with the same name -
                         */

                        filename_eq_dirname(dir_b, inotable_b, path_backup, pcrawl1->name); /* ++++++++++++++++++++++++++++++++++++++++++++ */

				        /* now we can: 
                         * 1)create the actual directory folder in backup, 
                         * 2)add the inode entry of this directory in the backup inode table,
				         * 3)add the directory node in the Backup tree with the same name as the directory node from source 
				         */

				        if(mkdir(path_backup, S_IRWXU | S_IRWXG | S_IRWXO) < 0) perror(path_backup);

				        /* acquire directory's information */
				        if(stat(path_backup, &info) == -1) perror(path_backup);

				        /* now we can insert the inode entry */
				        insert_entry(inotable_b, path_backup, info.st_ino, ctime(&info.st_mtime), info.st_size); 

				        /* there i should "link" the inode entry of "path_source" with the inode entry of "path_backup" */
				        link_source_backup(inotable_s, pcrawl1->inode, info.st_ino);

				        /* create the directory node to be inserted in the tree (+ adjust the links) */
				        newnode = create_node(pcrawl1->name, info.st_ino);
                        if(!pcrawl2->prev_dir_sibling) {        /* New directory node to be inserted at the start of the list */
                            pcrawl2->prev_dir_sibling = newnode;
                            newnode->next_dir_sibling = pcrawl2;
                            (*dir_b)->list_of_dir_children = newnode;
                        }
                        else {
                            node *prev = pcrawl2->prev_dir_sibling;

                            prev->next_dir_sibling = newnode;
                            pcrawl2->prev_dir_sibling = newnode;
                            newnode->prev_dir_sibling = prev;
                            newnode->next_dir_sibling = pcrawl2;
                        }
                        
                        ++(*dir_b)->dir_children;
					    sync_directories(&pcrawl1, &newnode, inotable_s, inotable_b, path_source, path_backup);

                        pcrawl1 = pcrawl1->next_dir_sibling;
                    }
				}
            } /* <- end of the if statement: if( (*dir_b)->dir_children > 0 && !flag ) */
            else {      /* Backup directory was missing directories OR ... */
                flag = 1;

                filename_eq_dirname(dir_b, inotable_b, path_backup, pcrawl1->name); /* ++++++++++++++++++++++++++++++++++++++++++++ */

				if(mkdir(path_backup, S_IRWXU | S_IRWXG | S_IRWXO) < 0) perror(path_backup);

				/* acquire directory's information */
				if(stat(path_backup, &info) == -1) perror(path_backup);

				/* now we can insert the inode entry */
				insert_entry(inotable_b, path_backup, info.st_ino, ctime(&info.st_mtime), info.st_size); 

				/* there i should "link" the inode entry of "path_source" with the inode entry of "path_backup" */
				link_source_backup(inotable_s, pcrawl1->inode, info.st_ino);

				/* create the directory node to be inserted in the tree (+ adjust the links) */
				newnode = create_node(pcrawl1->name, info.st_ino);
                if(!pcrawl2) 
                    (*dir_b)->list_of_dir_children = newnode;
                else {
                    pcrawl2->next_dir_sibling = newnode;
                    newnode->prev_dir_sibling = pcrawl2;                    
                }

                pcrawl2 = newnode;
                ++(*dir_b)->dir_children;

				sync_directories(&pcrawl1, &pcrawl2, inotable_s, inotable_b, path_source, path_backup);

                pcrawl1 = pcrawl1->next_dir_sibling;

                if(pcrawl1 == NULL) /* in this case, the directory treenode "newnode", which was added in backup, was deleted shortly after */
                    pcrawl2 = pcrawl2->next_dir_sibling;
            }
				
			free(path_source);
			path_source = NULL;
			free(path_backup);
			path_backup = NULL;
		} /* <- end of the while loop: while(pcrawl1 != NULL) */ 

		/* remove directories that do not exist in Source, but do exist in Backup */
        while(pcrawl2 != NULL) {
		    path_backup = create_path2(path_b, pcrawl2->name);

			printf("---Before remove directory---\n");

			/* function to delete the whole directory */
			remove_directory(&pcrawl2, inotable_b, path_backup, 0);

            printf("---After remove directory---\n");

            next = pcrawl2->next_dir_sibling;  
			delete_node(dir_b, pcrawl2->name, 'd');
			if(rmdir(path_backup) < 0) perror("rmdir");

			free(path_backup);
			path_backup = NULL;					

            pcrawl2 = next;
        }
        
	} /* <- end of the if statement: if( (*dir_s)->dir_children > 0 ) */
    else
        remove_unwanted_dirs(dir_b, inotable_b, path_b);


/******************************************************* FILES **************************************************************/


    printf("\n\nFILES!\n");

	int backup_ino;				
	char *buffer = NULL, *name_to_link = NULL;
    node *new_fnode;
	flag = 0; 
		

	/* Next, sync the File entities in the current tree level */
	if( (*dir_s)->file_children > 0) {

		pcrawl1 = (*dir_s)->list_of_file_children;
        pcrawl2 = (*dir_b)->list_of_file_children;

		while(pcrawl1 != NULL) {

			/* create the paths for source and backup */
			path_source = create_path2(path_s, pcrawl1->name);
			path_backup = create_path2(path_b, pcrawl1->name);
			printf("\nExists: %s\n", path_source);
			printf("Exists(?): %s\n", path_backup);


			/* check if the directory node in Backup has any file node children */
			if( (*dir_b)->file_children > 0 && !flag ) { 

				/* File name also exists in Backup */
				if(strcmp(pcrawl1->name, pcrawl2->name) == 0) {

					printf("Case 1) File names matched: %s\n", pcrawl2->name);

                    /* get "last modified" time and size of the file in Source */
					if(stat(path_source, &info) == -1) perror(path_source);
					size = info.st_size;
					strcpy(lastmod, ctime(&info.st_mtime));

					/* get "last modified" time and size of the file in Backup */
					if(stat(path_backup, &info) == -1) perror(path_backup);

					if( (size == info.st_size) && (strcmp(lastmod, ctime(&info.st_mtime)) == 0) ) { /* the 2 files are basically the same */

                        printf("Case 1a) The 2 files were the same!\n");

						link_source_backup(inotable_s, pcrawl1->inode, pcrawl2->inode);

                        /* for the next iteration */
                        pcrawl1 = pcrawl1->next_file_sibling;

                        if(!pcrawl2->next_file_sibling)
                            flag = 1;
                        else
                            pcrawl2 = pcrawl2->next_file_sibling;

					}
					else {  /* the 2 files are not the same */

                        printf("Case 1b) The 2 files were not the same!\n");

						printf("Unlink file %s with inode %d\n", pcrawl2->name, pcrawl2->inode);

						/* firstly, delete its entry from the inode table of the Backup directory */
						delete_entry(inotable_b, path_backup, pcrawl2->inode);

						/* next unlink the file */
						unlink(path_backup);

                        prev = pcrawl2->prev_file_sibling;
                        next = pcrawl2->next_file_sibling;

						/* lastly delete the filenode */
						delete_node(dir_b, pcrawl2->name, 'f');

						printf("%s has now %d file children\n", (*dir_b)->name, (*dir_b)->file_children);

						/* After the previous instruction pcrawl2 might point to GARBAGE!!! */

				        /* File entity DOES NOT have a copy in Backup */	
				        if( ( backup_ino = copy_exists(*inotable_s, *inotable_b, pcrawl1->inode, &name_to_link) ) == -1) { 
				        
				        	/* copy the file to the Backup directory */
				        	buffer = create_cpcommand(path_source, path_backup);
				        	if( system(buffer) == -1) perror("system cp");

				        	/* acquire file's information */
				        	if(stat(path_backup, &info) == -1) perror(path_backup);

				        	/* now we can insert the inode entry */
				        	printf("Created %s with i-node %ld\n", path_backup, info.st_ino);
				        	insert_entry(inotable_b, path_backup, info.st_ino, ctime(&info.st_mtime), info.st_size); 
				        	link_source_backup(inotable_s, pcrawl1->inode, info.st_ino);

                            free(buffer);
                            buffer = NULL;
				        }
				        else {
				        	/* File entity HAS a copy in Backup */

				        	printf("(Hard)Link %s (i-node %d) with %s\n", name_to_link, backup_ino, path_backup);

				        	/* link the file with another file in backup */
				        	if( link(name_to_link, path_backup) == -1 ) perror("link");

				        	if(stat(path_backup, &info) == -1) perror(path_backup);
				        	printf("Created %s with i-node %ld\n", path_backup, info.st_ino);

				        	/* now we can insert the inode entry */
				        	insert_entry(inotable_b, path_backup, info.st_ino, ctime(&info.st_mtime), info.st_size); 

				        	free(name_to_link);
				        	name_to_link = NULL;
				        }
                        
                        /* create a new file treenode after deleting (unlinking) the previous file name */
				        new_fnode = create_node(pcrawl1->name, info.st_ino);
                        ++(*dir_b)->file_children;
                        
                        /* insert the new file treenode (+ adjust the links) */
                        if(!prev && !next) {
                            flag = 1;
                            (*dir_b)->list_of_file_children = new_fnode;
                            if(pcrawl1->next_file_sibling == NULL)
                                pcrawl2 = NULL;
                            else
                                pcrawl2 = new_fnode;
                        }
                        else {
                            if(!prev) {
                                (*dir_b)->list_of_file_children = new_fnode;
                                new_fnode->next_file_sibling = next;
                                next->prev_file_sibling = new_fnode;
                                pcrawl2 = next;
                            }
                            if(!next) {
                                flag = 1;
                                prev->next_file_sibling = new_fnode;
                                new_fnode->prev_file_sibling = prev;

                                if(pcrawl1->next_file_sibling == NULL)
                                    pcrawl2 = NULL;
                                else
                                    pcrawl2 = new_fnode;
                            }
                            if(prev && next) {
                                prev->next_file_sibling = new_fnode;
                                new_fnode->prev_file_sibling = prev;
                                new_fnode->next_file_sibling = next;
                                next->prev_file_sibling = new_fnode;
                                pcrawl2 = next;
                            }
                        }

                        pcrawl1 = pcrawl1->next_file_sibling;
                    }
				}

				/* file names in Source and Backup are not the same 
				 * there are 2 cases: 
                 * 1) either strcmp(pcrawl1->name, pcrawl2->name) > 0, which means that pcrawl2 does not exist in Source, thus unlink it OR
				 * 2) strcmp(pcrawl1->name, pcrawl2->name) < 0, where pcrawl2 might exist in Source but later in its file children list due to 
                 *    the alphabetical order
				 */

				else {

					if(strcmp(pcrawl1->name, pcrawl2->name) > 0) {

                        printf("Case 2: File %s should not exist in backup (%s > %s)\n", pcrawl2->name, pcrawl1->name, pcrawl2->name);

						free(path_backup); 
                        path_backup = NULL;
						path_backup = create_path2(path_b, pcrawl2->name);

						printf("Unlink %s with inode %d with full pathname %s\n", pcrawl2->name, pcrawl2->inode, path_backup);

						/* firstly, delete its entry from the inode table of the backup directory */
						delete_entry(inotable_b, path_backup, pcrawl2->inode);

						/* next, unlink the file */
						unlink(path_backup);

                        prev = pcrawl2->prev_file_sibling; /* ++++++++++++++++++++++ */
                        next = pcrawl2->next_file_sibling; /* ++++++++++++++++++++++ */

						/* lastly delete the file treenode */
						delete_node(dir_b, pcrawl2->name, 'f');

						printf("%s now has %d file children\n", (*dir_b)->name, (*dir_b)->file_children);

                        if(!next) {
                            flag = 1;
                            pcrawl2 = prev;
                        }
                        else
                            pcrawl2 = next;

					}
					else {

						/* do not delete anything! - just add the file that does not exist in backup */
						printf("Case 3: File in Backup might exist later in Source! (%s < %s)\n", pcrawl1->name, pcrawl2->name);


				        /* File entity DOES NOT have a copy in Backup */	
				        if( ( backup_ino = copy_exists(*inotable_s, *inotable_b, pcrawl1->inode, &name_to_link) ) == -1) { 
				        
				        	/* copy the file to backup */
				        	buffer = create_cpcommand(path_source, path_backup);
				        	if( system(buffer) == -1 ) perror("system cp");

				        	/* acquire file's information */
				        	if(stat(path_backup, &info) == -1) perror(path_backup);

				        	/* now we can insert the inode entry */
				        	printf("Created %s with i-node %ld\n", path_backup, info.st_ino);
				        	insert_entry(inotable_b, path_backup, info.st_ino, ctime(&info.st_mtime), info.st_size); 
				        	link_source_backup(inotable_s, pcrawl1->inode, info.st_ino);

                            free(buffer);
                            buffer = NULL;
				        }
				        else {
				        	/* File entity HAS a copy in Backup */

				        	printf("(Hard)Link %s (i-node %d) with %s\n", name_to_link, backup_ino, path_backup);

				        	/* link the file with another file in backup */
				        	if( link(name_to_link, path_backup) == -1 ) perror("link");

				        	if(stat(path_backup, &info) == -1) perror(path_backup);
				        	printf("Created %s with i-node %ld\n", path_backup, info.st_ino);

				        	/* now we can insert the inode entry */
				        	insert_entry(inotable_b, path_backup, info.st_ino, ctime(&info.st_mtime), info.st_size); 

				        	free(name_to_link);
				        	name_to_link = NULL;
				        }
                        
				        new_fnode = create_node(pcrawl1->name, info.st_ino);

                        if(!pcrawl2->prev_file_sibling) {        /* New file treenode to be inserted at the start of the list */
                            pcrawl2->prev_file_sibling = new_fnode;
                            new_fnode->next_file_sibling = pcrawl2;
                            (*dir_b)->list_of_file_children = new_fnode;
                        }
                        else {
                            node *prev = pcrawl2->prev_file_sibling;

                            prev->next_file_sibling = new_fnode;
                            pcrawl2->prev_file_sibling = new_fnode;
                            new_fnode->prev_file_sibling = prev;
                            new_fnode->next_file_sibling = pcrawl2;
                        }
                        
                        ++(*dir_b)->file_children;
                        pcrawl1 = pcrawl1->next_file_sibling;
					}

				}

			} /* <- end of the if statement: if( (*dir_b)->file_children > 0 && !flag ) */
            else { /* <- Backup directory was missing files OR ... */

                flag = 1;

				/* File name does not exist in Backup */
				/* we need to check if the file entity in Source already has a copy in Backup */

				/* File entity DOES NOT have a copy in Backup */	
				if( ( backup_ino = copy_exists(*inotable_s, *inotable_b, pcrawl1->inode, &name_to_link) ) == -1) { 
				
					/* copy the file to backup */
					buffer = create_cpcommand(path_source, path_backup);
					if( system(buffer) == -1 ) perror("system cp");

					/* acquire file's information */
					if(stat(path_backup, &info) == -1) perror(path_backup);

					/* now we can insert the inode entry */
					printf("Created %s with i-node %ld\n", path_backup, info.st_ino);
					insert_entry(inotable_b, path_backup, info.st_ino, ctime(&info.st_mtime), info.st_size); 
					link_source_backup(inotable_s, pcrawl1->inode, info.st_ino);

                    free(buffer);
                    buffer = NULL;
				}
				else {
					/* File entity HAS a copy in Backup */

					printf("Link %s with %s which has inode %d\n", name_to_link, path_backup, backup_ino);

					/* link the file with another file in backup */
					if( link(name_to_link, path_backup) == -1 ) perror("link");

					if(stat(path_backup, &info) == -1) perror(path_backup);
					printf("Created %s with inode %ld\n", path_backup, info.st_ino);

					/* now we can insert the inode entry */
					insert_entry(inotable_b, path_backup, info.st_ino, ctime(&info.st_mtime), info.st_size); 
					free(name_to_link);
					name_to_link = NULL;
				}


				/* create the file treenode to be inserted in the tree (+ adjust the links) */
				new_fnode = create_node(pcrawl1->name, info.st_ino);

                if(!pcrawl2) 
                    (*dir_b)->list_of_file_children = new_fnode;
                else {
                    pcrawl2->next_file_sibling = new_fnode;
                    new_fnode->prev_file_sibling = pcrawl2;                    
                }

                pcrawl2 = new_fnode;
                ++(*dir_b)->file_children;

                pcrawl1 = pcrawl1->next_file_sibling;

                if(pcrawl1 == NULL) /* in this case, the file treenode "new_fnode", which was added in backup, was deleted shortly after */
                    pcrawl2 = pcrawl2->next_file_sibling;
			}

			free(path_source);
			path_source = NULL;
			free(path_backup);
			path_backup = NULL;
		} /* <- end of the while loop: while(pcrawl1 != NULL) */


		/* Source Directory does not have any other files. If Backup Directory happens to include more files, 
         * we need to physically unlink them and remove their data from the data structures (tree, i-node table)
		 */

        while(pcrawl2 != NULL) {

		    path_backup = create_path2(path_b, pcrawl2->name);
			printf("Unlink %s with inode %d with full pathname %s\n", pcrawl2->name, pcrawl2->inode, path_backup);

			/* firstly, delete its entry from the inode table of the Backup directory */
			delete_entry(inotable_b, path_backup, pcrawl2->inode);

            /* next, unlink the file */
			if(unlink(path_backup) < 0) perror(path_backup);

            next = pcrawl2->next_file_sibling; 

			/* lastly delete the file treenode */
			delete_node(dir_b, pcrawl2->name, 'f');

			printf("%s now has %d file children\n", (*dir_b)->name, (*dir_b)->file_children);

			free(path_backup);
			path_backup = NULL;					

            pcrawl2 = next;
        }

	} /* <- end of the if statement: if( (*dir_s)->file_children > 0 ) */
    else
        remove_unwanted_files(dir_b, inotable_b, path_b);
}


void add_watch_descriptors(watch_descriptor **listhead, int inotifyfd, node *parent_directory, char *recpath, char *root_dir_name) {

	char *pathname, *add_wd_path;	
	int wd;
	watch_descriptor *newnode;

	if(*listhead == NULL) {		/* add a watch descriptor on the root/Source directory as well */
		wd = inotify_add_watch(inotifyfd, root_dir_name, IN_CREATE | IN_ATTRIB | IN_MODIFY | \
								  	IN_CLOSE_WRITE | IN_DELETE | IN_DELETE_SELF \
									| IN_MOVED_FROM | IN_MOVED_TO);

		if(wd == -1) {
			perror("inotify add watch at Source directory");
			exit(-1);
		}

		printf("Watching %s using wd %d\n", parent_directory->name, wd);
		newnode = create_watch_descriptor(wd, root_dir_name);	
		insert_watch_descriptor(listhead, newnode);
	}
	if(parent_directory->dir_children > 0) {    /* add a watch descriptor on each directory inside the Source directory */
		
		node *pcrawl = parent_directory->list_of_dir_children;
		while(pcrawl != NULL) {

			/* at the first recursive call - recpath is still NULL - */
			if(recpath == NULL) {
				pathname = (char *)malloc(strlen(pcrawl->name)  + 1);
				strcpy(pathname, pcrawl->name);
			}
			else {
				pathname = create_path2(recpath, pcrawl->name);
			}
			/* in order to add a watch descriptor, we need the full pathname */
			add_wd_path = create_path2(root_dir_name, pathname);
			wd = inotify_add_watch(inotifyfd, add_wd_path, IN_CREATE | IN_ATTRIB | IN_MODIFY | \
								  	IN_CLOSE_WRITE | IN_DELETE | IN_DELETE_SELF \
									| IN_MOVED_FROM | IN_MOVED_TO);
			if(wd == -1) {
				perror("inotify add watch");
				exit(-1);
			}
			printf("Watching %s using wd %d\n", add_wd_path, wd);
			newnode = create_watch_descriptor(wd, pathname);	
			insert_watch_descriptor(listhead, newnode);

            /* recursively add wds on each directory in the hierarchy */
			add_watch_descriptors(listhead, inotifyfd, pcrawl, pathname, root_dir_name);    

			free(pathname);
			pathname = NULL;
			free(add_wd_path);
			add_wd_path = NULL;
			pcrawl = pcrawl->next_dir_sibling;
		}
	}
}


void remove_watch_descriptors(watch_descriptor **listhead, int inotify_fd) {

	watch_descriptor *pcrawl  = *listhead;
	watch_descriptor *temp = NULL;

	while(pcrawl != NULL) {

		inotify_rm_watch(inotify_fd, pcrawl->wd);
		temp = pcrawl;
		pcrawl = pcrawl->next;
		free(temp->pathname);
		free(temp);
	}
	*listhead = NULL;
}


int get_inode(struct inotify_event *event, watch_descriptor *listhead, char root_name[35], char modified_name[35]) {

	char *parentdir = NULL, *source_path = NULL;
	struct stat info;

    printf("(inside get_inode)\n");

	parentdir = find_wd(listhead, event->wd);
	if(strcmp(parentdir, root_name) != 0)
		source_path = create_path3(root_name, parentdir, modified_name);
	else
		source_path = create_path2(root_name, modified_name);

    free(parentdir);

	if(stat(source_path, &info) == -1) {    /* in_create case */
        perror(source_path);
        free(source_path);

        return -1;
    }

    free(source_path);

	return info.st_ino;
}


void cleanup(node **root_source, node **root_backup, inode_entry **inotable_source, inode_entry **inotable_backup, \
                watch_descriptor **lhead, int inotify_fd) {

    delete_dir_entity(root_source, inotable_source, (*root_source)->name, (*root_source)->name, 1);
    free(*root_source);
    delete_dir_entity(root_backup, inotable_backup, (*root_backup)->name, (*root_backup)->name, 1);
    free(*root_backup);
    remove_watch_descriptors(lhead, inotify_fd);    
}


/***********************************************************************************************************************************/

/* for testing purposes */
void print_directory_contents(node *dir) {

	node *pcrawl = dir;

	printf("Inside %s: -----------------------\n", dir->name);
	pcrawl = dir->list_of_dir_children;
	/* dir includes other directories */
	while(pcrawl != NULL) {
		printf("----- %s: file children = %d, dir children = %d\n", pcrawl->name, pcrawl->file_children, pcrawl->dir_children);
		print_directory_contents(pcrawl);
		pcrawl = pcrawl->next_dir_sibling;
	}
	printf("Inside %s: -----------------------\n", dir->name);
	pcrawl = dir->list_of_file_children;
	/* dir includes other files */
	while(pcrawl != NULL) {
		printf("----- %s\n", pcrawl->name);
		pcrawl = pcrawl->next_file_sibling;
	}
}


void print_inotable(inode_entry *inotable) {

	inode_entry *itraverse = inotable;

	while(itraverse != NULL) {
		printf("Inode: %d\nLast modified: %sBackup inode: %d\nSize: %d\nLinked files: %d", \
		     						itraverse->inode_number, itraverse->last_modified, \
		     						itraverse->backup_inode, itraverse->size, \
		     						itraverse->num_of_linked_files);
		if(itraverse->num_of_linked_files > 1)
			printf("(%s, %s)\n", itraverse->head->entity_name, itraverse->head->next->entity_name);
		else
			printf("(%s)\n", itraverse->head->entity_name);
		putchar('\n');
		itraverse = itraverse->next;
	}
}
