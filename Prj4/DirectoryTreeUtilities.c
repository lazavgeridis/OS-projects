/* DirectoryTreeUtilities.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>  	/* for unlink and rmdir */
#include <sys/stat.h>	/* for struct stat */	
#include <sys/types.h>
#include <sys/inotify.h>

#include "DirectoryTreeStructures.h"

/* forward decleration of create_node */
node *create_node (char *, int, char);

name_node *create_name_node(char *entity_path_name) {

	name_node *newname_node = (name_node *)malloc( sizeof(name_node) );
    if(!newname_node) {
        fprintf(stderr, "Insufficient memory!\n");
        exit(EXIT_FAILURE);
    }
	newname_node->entity_name = (char *)malloc(strlen(entity_path_name) + 1);
    if(!newname_node->entity_name) {
        fprintf(stderr, "Insufficient memory!\n");
        exit(EXIT_FAILURE);
    }
	strcpy(newname_node->entity_name, entity_path_name); 
	newname_node->next = NULL;

	return newname_node;
}


inode_entry *create_inode_entry(int inode, char *mdate, off_t size) {

	inode_entry *new_inode_entry = (inode_entry *)malloc( sizeof(inode_entry) );
    if(!new_inode_entry) {
        fprintf(stderr, "Insufficient memory!\n");
        exit(EXIT_FAILURE);
    }
	new_inode_entry->inode_number = inode;
	strcpy(new_inode_entry->last_modified, mdate);
	new_inode_entry->backup_inode = -1;
	new_inode_entry->size = (unsigned int)size;
	new_inode_entry->num_of_linked_files = 1;
	new_inode_entry->head = NULL;
	new_inode_entry->next = NULL;

	return new_inode_entry;
}


watch_descriptor *create_watch_descriptor(int wd, char *pathname) {

	watch_descriptor *newwd = (watch_descriptor *)malloc( sizeof(watch_descriptor) );
    if(!newwd) {
        fprintf(stderr, "Insufficient memory!\n");
        exit(EXIT_FAILURE);
    }
	newwd->wd = wd;
	newwd->pathname = (char *)malloc(strlen(pathname) + 1);
	strcpy(newwd->pathname, pathname);
	newwd->next = NULL;

	return newwd;
}


char *create_path2(char *prefix, char *last_part) {

	char *fullpath = (char *)malloc(strlen(prefix) + strlen(last_part) + 2);
	strcpy(fullpath, prefix);
	strcat(fullpath, "/");
	strcat(fullpath, last_part);

	return fullpath;
}


char *create_path3(char *root_dir, char *prefix, char *last_part) {

	char *fullpath = (char *)malloc(strlen(root_dir) + strlen(prefix) + strlen(last_part) + 3);
	strcpy(fullpath, root_dir);
	strcat(fullpath, "/");
	strcat(fullpath, prefix);
	strcat(fullpath, "/");
	strcat(fullpath, last_part);

	return fullpath;
}


char *create_cpcommand(char *from, char *to) {

	char *command = (char *)malloc(strlen("cp") + strlen(from) + strlen(to) + 3);
	strcpy(command, "cp ");
	strcat(command, from);
	strcat(command, " ");
	strcat(command, to);

	return command;
}


char *find_wd(watch_descriptor *listhead, int wd) { /* map a watch descriptor to a relative path */

	watch_descriptor *pcrawl = listhead;
	char *dir_pathname = NULL;

    while(pcrawl->wd != wd) pcrawl = pcrawl->next;

	dir_pathname = (char *)malloc(strlen(pcrawl->pathname) + 1);
	strcpy(dir_pathname, pcrawl->pathname);

	return dir_pathname;
}


int compare_mtimes(inode_entry *inotable, int inode, char *mtime) {

	inode_entry *pcrawl = inotable;

    printf("Searching for i-node %d\n", inode);

	while(pcrawl->inode_number != inode) 
        pcrawl = pcrawl->next;
	if(strcmp(pcrawl->last_modified, mtime) != 0)
		return 1;

	return 0;
}


int inode_exists(inode_entry *inotable, int inode) {

	inode_entry *pcrawl = inotable;
	while(pcrawl != NULL) {

		if(pcrawl->inode_number == inode) return 0; 
		pcrawl = pcrawl->next;
	}

	return 1;
}


int copy_exists(inode_entry *inotable_s, inode_entry *inotable_b, int inode, char **name_link) {


	inode_entry *pcrawl = inotable_s;

	while(pcrawl->inode_number != inode) 
        pcrawl = pcrawl->next;

	if(pcrawl->backup_inode == -1) return -1;

	if(pcrawl->num_of_linked_files > 1) {       /* since >1 file names share the same i-node number, they are hardlinks */
		int backup_ino = pcrawl->backup_inode;
		inode_entry *pcrawl2 = inotable_b;

		while(pcrawl2->inode_number != backup_ino) 
			pcrawl2 = pcrawl2->next;
		
        printf("%s\n", pcrawl2->head->entity_name);
		*name_link = (char *)malloc(strlen(pcrawl2->head->entity_name) + 1);
		strcpy(*name_link, pcrawl2->head->entity_name);

		return backup_ino;
	}

    return -1;
}


void insert_entry(inode_entry **inotable, char *pathname, int inode, char *lastmod, off_t size) {

	name_node *nmnode;
	inode_entry *inoentry, *pcrawl = *inotable;

	/* construct a new name_node with name = pathname */
	nmnode = create_name_node(pathname);

	/* in the case of file entities, a file might have the same inode number with another file entity 
	 * in this case, we need to check the inode table first to check if an inode entry with the same inode number already exists
	 */
	
	if(inode_exists(*inotable, inode) == 1) {	/* an inode entry with inode number = inode DOES NOT exist */

		/* construct a new inode_entry */
		inoentry = create_inode_entry(inode, lastmod, size);
		inoentry->head = nmnode;
		/* now add the inode entry at the the start of the inode table list */
		inoentry->next = (*inotable);
		(*inotable) = inoentry;
	}
	else {						/* an inode entry with inode number = inode DOES exist */
		while(pcrawl->inode_number != inode)
		       pcrawl = pcrawl->next;
		nmnode->next = pcrawl->head;
		pcrawl->head = nmnode;
		pcrawl->num_of_linked_files++;
	}
}


void delete_entry(inode_entry **inotable, char *entity_name, int inode) {

	inode_entry *prev = *inotable;
	inode_entry *cur = prev->next;
	name_node *prevnm, *curnm;

    printf("(delete_entry): Looking for entity with name %s\n", entity_name);

	/* in case the inode entry with "inode_number = inode" we are looking for is the 1st one in the inode table */
	if(inode == prev->inode_number) {

		printf("(delete_entry): First entry in the inode table!\n");

		if(prev->num_of_linked_files == 1) {

			*inotable = prev->next;
			free(prev->head->entity_name);
			free(prev->head);
			free(prev);
		}
		else {
			/* prev->num_of_linked_files >= 2 */				
			prevnm = prev->head;
			curnm = prev->head->next;

			if(strcmp(entity_name, prevnm->entity_name) == 0) {
				prev->head = curnm;
				free(prevnm->entity_name);
				free(prevnm);
				--prev->num_of_linked_files;
			}
			else {

				while(curnm != NULL) {
					if(strcmp(entity_name, curnm->entity_name) == 0) {
						prevnm->next = curnm->next;
						free(curnm->entity_name);
						free(curnm);
						--prev->num_of_linked_files;

						break;
					}
					prevnm = curnm;
					curnm = curnm->next;
				}
			}
		}
	}
	else {
		
		printf("(delete_entry): Not the first entry in the inode table!\n");

		while(cur != NULL) {

			if(cur->inode_number == inode) {
                printf("Found %s 's inode entry\n", entity_name);

				/* cur is the inode table entry, which includes the entity name */
				if(cur->num_of_linked_files == 1) {		
					/* the whole inode entry must be deleted, since only the name we are about to delete is linked with this inode number */
					/* adjust the links */
					prev->next = cur->next;
					/* free name_node name */
					free(cur->head->entity_name);
					/* free the name_node */
					free(cur->head);
					/* free the current inode entry */
					free(cur);

				}	
				else {
					/* prev->num_of_linked_files >= 2 */				
					/* we must locate the name_node that contains the name of the entity we want to delete */
					prevnm = cur->head;
					curnm = cur->head->next;
					if(strcmp(prevnm->entity_name, entity_name) == 0) {	
						/* name_node to be deleted is the first in the list */
						/* free name_node name */
						free(prevnm->entity_name);
						cur->head = curnm;
						free(prevnm);
						--cur->num_of_linked_files;

					}
					else {
						while(curnm != NULL) {
							if(strcmp(curnm->entity_name, entity_name) == 0) {
								/* free name_node name */
								free(curnm->entity_name);
								prevnm->next = curnm->next;
								/* free name_node */
								free(curnm);
								--cur->num_of_linked_files;

								break;
							}
							prevnm = curnm;
							curnm = curnm->next;
						}
					}
				}
				break;
			}
			prev = cur;
			cur = cur->next;
		}
	}
}


int update_entry_date(inode_entry **inotable, int old_inode, int new_inode, char *newmtime) {

	/* 1. locate the i-node entry
     * 2. copy all the fields of the i-node entry on to the new i-node entry, just change the "last modified" date
     * 3. replace the old i-node entry with the new one
     */
	printf("(update_entry_date): Searching for inode %d\n", old_inode);

	inode_entry *prev = *inotable;
	inode_entry *cur = (*inotable)->next;

	if(prev->inode_number == old_inode) {	/* entry to be updated is the first one in the list */

		inode_entry *new_entry = create_inode_entry(new_inode, newmtime, prev->size);
		new_entry->num_of_linked_files = prev->num_of_linked_files;
		new_entry->backup_inode = prev->backup_inode;
		new_entry->head = prev->head;
		new_entry->next = (*inotable)->next;
		*inotable = new_entry;

		printf("(update_entry_date): Entry at the top of the list: %d\n", (*inotable)->inode_number);
		free(prev);

		return new_entry->backup_inode;
	}
	else {					/* traverse the list to find the entry */
		while(cur->inode_number != old_inode) {
			prev = cur;
			cur = cur->next;
		}
		inode_entry *new_entry = create_inode_entry(new_inode, newmtime, cur->size);
		new_entry->num_of_linked_files = cur->num_of_linked_files;
		new_entry->backup_inode = cur->backup_inode;
		new_entry->head = cur->head;
		new_entry->next = cur->next;
		prev->next = new_entry;

		printf("(update_entry_date): Entry added: %d\n", prev->next->inode_number);
		free(cur);

		return new_entry->backup_inode;
	}
}


int update_entry_size(inode_entry **inotable, int inode, int newsize) {

	inode_entry *prev = *inotable;
	inode_entry *cur = (*inotable)->next;

	printf("Searching for inode %d\n", inode);

	if(prev->inode_number == inode) {	/* entry to be updated is the first one in the list */

		inode_entry *new_entry = create_inode_entry(inode, prev->last_modified, newsize);
		new_entry->num_of_linked_files = prev->num_of_linked_files;
		new_entry->backup_inode = prev->backup_inode;
		new_entry->head = prev->head;
		new_entry->next = (*inotable)->next;
		*inotable = new_entry;
		printf("Entry at the top of the list: %d\n", (*inotable)->inode_number);
		free(prev);

		return new_entry->backup_inode;
	}
	else {					/* traverse the list to find the entry */
		while(cur->inode_number != inode) {
			prev = cur;
			cur = cur->next;
		}
		inode_entry *new_entry = create_inode_entry(inode, cur->last_modified, newsize);
		new_entry->num_of_linked_files = cur->num_of_linked_files;
		new_entry->backup_inode = cur->backup_inode;
		new_entry->head = cur->head;
		new_entry->next = cur->next;
		prev->next = new_entry;
		printf("Entry added with inode: %d\n", prev->next->inode_number);
		free(cur);

		return new_entry->backup_inode;
	}
}


int delete_node(node **dirnode, char *name, char entity_type) {

	int ino;
	node *prev, *next, *temp;

	/* the case of File treenode deletion */
	if(entity_type == 'f') {

		temp = (*dirnode)->list_of_file_children;

		if(strcmp(name, temp->name) == 0) {     /* First file treenode in the list is the one we are deleting */
			ino = temp->inode;
			(*dirnode)->list_of_file_children = temp->next_file_sibling;
            if( (*dirnode)->list_of_file_children != NULL )
                (temp->next_file_sibling)->prev_file_sibling = NULL;
			free(temp);
			--(*dirnode)->file_children;

			return ino;
		}
		else {

            temp = temp->next_file_sibling;
			while(strcmp(name, temp->name) != 0) 
                temp = temp->next_file_sibling;
			
            ino = temp->inode;
            prev = temp->prev_file_sibling; 
            next = temp->next_file_sibling;

            if(!next) {
                prev->next_file_sibling = NULL;
            }
            else {
                prev->next_file_sibling = next;
                next->prev_file_sibling = prev;
            }

			free(temp);
			--(*dirnode)->file_children;

			return ino;

		}
	}
	else {	/* the case of Directory treenode deletion */

		temp = (*dirnode)->list_of_dir_children;

		if(strcmp(name, temp->name) == 0) {
			(*dirnode)->list_of_dir_children = temp->next_dir_sibling;
            if( (*dirnode)->list_of_dir_children != NULL )
                (temp->next_dir_sibling)->prev_dir_sibling = NULL;
			free(temp);
			--(*dirnode)->dir_children;
		}
		else {
            temp = temp->next_dir_sibling;

			while(strcmp(name, temp->name) != 0) 
                temp = temp->next_dir_sibling;

            prev = temp->prev_dir_sibling; 
            next = temp->next_dir_sibling;

            if(!next) {
                prev->next_dir_sibling = NULL;
            }
            else {
                prev->next_dir_sibling = next;
                next->prev_dir_sibling = prev;
            }

			free(temp);
			--(*dirnode)->dir_children;
		}
	}
}


/* finds if a file treenode in the specified Backup directory has the name "dirname" */
void filename_eq_dirname(node **backup_dirnode, inode_entry **backup_inotable, char *path, char *dirname) {  

    node *temp = (*backup_dirnode)->list_of_file_children;
   
    while(temp != NULL) {
        
        if(strcmp(dirname, temp->name) == 0) {
   
            /* a file node with the same name exists. We need to remove this entity */
            /* firstly, delete its entry from the inode table of the backup directory */
            delete_entry(backup_inotable, path, temp->inode);
   
            /* next unlink the file */
            unlink(path);
   
            /* lastly delete the filenode */
            delete_node(backup_dirnode, temp->name, 'f');    /* delete a file treenode */
   
            break;
        }
        temp = temp->next_file_sibling;
    }
}


void link_source_backup(inode_entry **inotable_source, int source_ino, int backup_ino) {

	inode_entry *pcrawl = *inotable_source;

	while(pcrawl != NULL) {

		if(source_ino == pcrawl->inode_number) {
			pcrawl->backup_inode = backup_ino;

			break;
		}
		pcrawl = pcrawl->next;
	}
}


/* insert a new watch_descriptor node at the start of the watch descriptor "mappings" list */
void insert_watch_descriptor(watch_descriptor **listhead, watch_descriptor *newnode) {
	
	if( (*listhead) == NULL)
		*listhead = newnode;
	else {
		newnode->next = *listhead;
		*listhead = newnode;
	}
}


/* recursively search for the correct depth to add the file treenode in the tree + insert the file's inode entry
 * return the actual i-node number of the newly created file
 */
int insert_file_entity(struct inotify_event *event, node **dir, inode_entry **inotable, char *fullpath, char *recpath) {

	int rv;

    printf("Looking for %s\n", fullpath);
    printf("Currently at %s\n", recpath);

	/* we are inside the directory where the file node should be added */
	if(strcmp(fullpath, recpath) == 0) {

		char *pathname = create_path2(fullpath, event->name);
		struct stat info;
		if(stat(pathname, &info) == -1) perror(pathname);

		insert_entry(inotable, pathname, info.st_ino, ctime(&info.st_mtime), info.st_size);	/* insert new file's inode entry */ 
		node *newnode = create_node(event->name, info.st_ino, 'f');

		/* if it has 0 file children? */
        /* insert new file's treenode - sorted insert ? - */
        if( (*dir)->file_children == 0 ) {
		    (*dir)->list_of_file_children = newnode;	/* insert the new file treenode at the start of the file_children list */
        }
        else {
            node *first = (*dir)->list_of_file_children;
            first->prev_file_sibling = newnode;
		    newnode->next_file_sibling = first;
            (*dir)->list_of_file_children = newnode;
        }
		++(*dir)->file_children;		/* increment number of file children of the current (parent) directory treenode */

		printf("Added %s with inode %ld\n", (*dir)->list_of_file_children->name, info.st_ino);

		free(pathname);

		return info.st_ino;	/* return the inode number of the newly created file */
	}
	else {  /* the directory where the file node should be added is "deeper" into the directory tree */
		node *pcrawl = (*dir)->list_of_dir_children;
		while(pcrawl != NULL) {
			char *newpath = create_path2(recpath,pcrawl->name);
			rv = insert_file_entity(event, &pcrawl, inotable, fullpath, newpath);	

			free(newpath);
			newpath = NULL;
			if(rv != 0) return rv;

			pcrawl = pcrawl->next_dir_sibling;
		}
	}

	return 0;
}


/* recursively search for the correct depth to add the directory treenode in the tree + insert the directory's inode entry
 * return the actual i-node number of the newly created directory
 */
int insert_dir_entity(struct inotify_event *event, node **dir, inode_entry **inotable, char *fullpath, char *recpath) {

	/* we are inside the directory where the directory node should be added */
	if(strcmp(fullpath, recpath) == 0) {

		char *pathname = create_path2(fullpath, event->name);
		struct stat info;
		if(stat(pathname, &info) == -1) perror(pathname);

		insert_entry(inotable, pathname, info.st_ino, ctime(&info.st_mtime), info.st_size);	/* insert new directory's inode entry */
		node *newnode = create_node(event->name, info.st_ino, 'd');

        /* sorted insert ? */
        if( (*dir)->dir_children == 0 ) {
		    (*dir)->list_of_dir_children = newnode;	/* insert the new file treenode at the start of the file_children list */
        }
        else {
            node *first = (*dir)->list_of_dir_children;
            first->prev_dir_sibling = newnode;
		    newnode->next_dir_sibling = first;
            (*dir)->list_of_dir_children = newnode;
        }
		++(*dir)->dir_children;		/* increment number of directory treenode children of the current (parent) directory */

		printf("Added %s with inode %ld\n", pathname, info.st_ino);

		free(pathname);

		return info.st_ino;	/* return the inode number of the newly created directory */
	}
	else {  /* the directory where the directory node should be added is "deeper" into the directory tree */
		node *pcrawl = (*dir)->list_of_dir_children;
		while(pcrawl != NULL) {
			char *newpath = create_path2(recpath, pcrawl->name);
			int rv = insert_dir_entity(event, &pcrawl, inotable, fullpath, newpath);

			free(newpath);
			newpath = NULL;
			if(rv != 0) return rv;

			pcrawl = pcrawl->next_dir_sibling;
		}
	}

	return 0;
}


int delete_file_entity(node **dir, inode_entry **inotable, char *recpath, char *fullpath, char *event_name) {

    char *path = NULL;

	if(strcmp(recpath, fullpath) == 0) {	/* the file entity we wish to delete is on the "top level" of the directory structure */
		printf("%s has %d file children before file treenode deletion\n", (*dir)->name, (*dir)->file_children);
		int ino = delete_node(dir, event_name, 'f');	/* delete the file treenode */
		printf("%s has %d file children after file node deletion\n", (*dir)->name, (*dir)->file_children);
		path = create_path2(fullpath, event_name);
		delete_entry(inotable, path, ino); 	/* delete the file's i-node entry */

		//if(unlink(path) < 0) perror("unlink inside delete_file_entity");
		free(path);

		return 0;
	}
	else {  /* the file entity we wish to delete is "deeper" in the directory structure */
		node *pcrawl = (*dir)->list_of_dir_children;
		while(pcrawl != NULL) {
			path = create_path2(recpath, pcrawl->name);
            int rv = delete_file_entity(&pcrawl, inotable, path, fullpath, event_name);

			free(path);
			path = NULL;

            if(rv == 0) return rv;

			pcrawl = pcrawl->next_dir_sibling;
		}
	}

	return 1;
}


void delete_file_list(node **dir, inode_entry **inotable, char *path, int cleanup) {

    struct stat info;
	char *pathname;
	node *temp = NULL, *next = (*dir)->list_of_file_children;

	printf("(delete_file_list): %s has %d file children before deletion\n", (*dir)->name, (*dir)->file_children);

	while(next != NULL) {

		pathname = (char *)malloc(strlen(path) + strlen(next->name) + 2);
		strcpy(pathname, path);
		strcat(pathname, "/");
		strcat(pathname, next->name);
        if(stat(pathname, &info) < 0) perror(pathname);

        if(!cleanup)
		    if( unlink(pathname) < 0 ) perror(pathname);

		delete_entry(inotable, pathname, info.st_ino);
		temp = next;
		next = next->next_file_sibling;
		free(temp);
		free(pathname);
		pathname = NULL;
		--(*dir)->file_children;
	}

	printf("(delete_file_list): %s has %d file children after deletion\n", (*dir)->name, (*dir)->file_children);
	(*dir)->list_of_file_children = NULL;
}


void remove_unwanted_files(node **dir, inode_entry **inotable, char *path) {

    char *path_backup = NULL;
    node *pcrawl = NULL, *next = NULL;

    pcrawl = (*dir)->list_of_file_children;
    while(pcrawl != NULL) {
        path_backup = create_path2(path, pcrawl->name);
        delete_entry(inotable, path_backup, pcrawl->inode);
        if(unlink(path_backup) < 0) perror(path_backup);
        next = pcrawl->next_file_sibling;
        delete_node(dir, pcrawl->name, 'f');

        free(path_backup);
        path_backup = NULL;
        pcrawl = next;
    }
}


void remove_directory(node **dir, inode_entry **inotable, char *path, int cleanup) {

	char *pathname = NULL;
	node *temp = NULL;

	printf("**** Inside remove_directory ****\n");
	printf("Pathname is %s and regular name is %s\n", path, (*dir)->name);

	/* if there are any directory "children" under the current directory, delete those first */
	while( (*dir)->dir_children != 0 ) {

		temp = (*dir)->list_of_dir_children;
		pathname = create_path2(path, temp->name);
		remove_directory(&temp, inotable, pathname, cleanup);

		/* lastly delete this directory node */
		delete_node(dir, temp->name, 'd');
        printf("(remove_directory): Before rmdir on %s\n", pathname);
        if(!cleanup)
		    if(rmdir(pathname) < 0) perror("rmdir inside remove_directory");

		printf("(remove_directory): %s has %d dir children after deletion\n", (*dir)->name, (*dir)->dir_children);

		free(pathname);
		pathname = NULL;	
	}

	/* we must also delete the file "children" under the current directory */
	if( (*dir)->file_children > 0) {
		delete_file_list(dir, inotable, path, cleanup);
		if( (*dir)->list_of_file_children == NULL ) printf("(remove_directory): %s has now 0 file children!\n", (*dir)->name);
	}

	if(*inotable != NULL)
		delete_entry(inotable, path, (*dir)->inode);
}


void remove_unwanted_dirs(node **dir, inode_entry **inotable, char *path) {

    char *path_backup = NULL;
    node *next = NULL, *pcrawl = NULL;

    pcrawl = (*dir)->list_of_dir_children;
    while(pcrawl != NULL) {
        path_backup = create_path2(path, pcrawl->name);
        remove_directory(&pcrawl, inotable, path_backup, 0);
        next = pcrawl->next_dir_sibling;
        delete_node(dir, pcrawl->name, 'd');
        if(rmdir(path_backup) < 0) perror(path_backup);

        free(path_backup);
        path_backup = NULL;
        pcrawl = next;
    }
}


int delete_dir_entity(node **dir, inode_entry **inotable, char *recpath, char *fullpath, int cleanup) {

	char *newpath = NULL;

	printf("(delete_dir_entity): Inside %s:\n", recpath);

    /* User deleted the whole Source/Backup directory */
    if( !strcmp(recpath, fullpath) ) {
		printf("(delete_dir_entity): Before remove_directory on %s\n", recpath);
		remove_directory(dir, inotable, fullpath, cleanup);
		printf("(delete_dir_entity): After remove_directory on %s\n", recpath);
		if( (*dir)->dir_children > 0 )  /* for the case in which we are deleting the root treenode */
            delete_node(dir, recpath, 'd');

        return 0;
    }

    /* User deleted one of the directories at the top level of the directory structure */
	node *pcrawl = (*dir)->list_of_dir_children;
	while(pcrawl != NULL) {
		printf("%s\n", pcrawl->name); 
		newpath = create_path2(recpath, pcrawl->name);
		if(strcmp(newpath, fullpath) == 0) {
			printf("(delete_dir_entity): Before remove_directory on %s\n", newpath);
			remove_directory(&pcrawl, inotable, fullpath, cleanup);
			printf("(delete_dir_entity): After remove_directory on %s\n", newpath);
			delete_node(dir, pcrawl->name, 'd');

			free(newpath);

			return 0;
		}
		free(newpath);
		newpath = NULL;

		pcrawl = pcrawl->next_dir_sibling;
	}

    /* User deleted one of the directories that are "deeper" into the directory structure */
	pcrawl = (*dir)->list_of_dir_children;
	while(pcrawl != NULL) {
		newpath = create_path2(recpath, pcrawl->name);

		if(delete_dir_entity(&pcrawl, inotable, newpath, fullpath, cleanup) == 0) {
			free(newpath);

			return 0;
		}
		free(newpath);
		newpath = NULL;
		pcrawl = pcrawl->next_dir_sibling;
	}

	return 1;
}
