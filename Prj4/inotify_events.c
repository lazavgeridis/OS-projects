/* inotify_events.c */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/inotify.h>
#include <sys/stat.h>

#include "DirectoryTreeUtilities.h"



void in_create(struct inotify_event *event, int inotifyfd, watch_descriptor **listhead, node **source_root, inode_entry **source_inotable, node **backup_root, inode_entry **backup_inotable) {

	char *parentdir = NULL, *source_path = NULL, *backup_path = NULL, *to = NULL;


	/* in_create event was generated by a file entity */
	if( (event->mask & IN_ISDIR) == 0) {

		parentdir = find_wd(*listhead, event->wd);	/* parentdir is the directory path where the file was created */

		/* check if the event was generated from the 1st level of the directory tree */
		if(strcmp(parentdir, (*source_root)->name) != 0)
			source_path = create_path2( (*source_root)->name, parentdir);
		else {
			source_path = (char *)malloc(strlen( (*source_root)->name ) + 1);
			strcpy(source_path, (*source_root)->name);
		}

        /* update the data structures regarding the Source directory (Source tree, Source inode table) */
		int sino = insert_file_entity(event, source_root, source_inotable, source_path, (*source_root)->name); 
		free(source_path);

		printf("(Inside in_create): Added file entity with inode %d in Source\n", sino);

		int bino;
		char *name_to_link = NULL;

		/* the new file created in Source DOES NOT have a copy in Backup */
		if( ( bino = copy_exists(*source_inotable, *backup_inotable, sino, &name_to_link) ) == -1) {

			char *from = NULL, *command = NULL;

			printf("No Copy Exists\n");

			if(strcmp(parentdir, (*source_root)->name) != 0) {
				from = create_path3( (*source_root)->name, parentdir, event->name );
				to = create_path3( (*backup_root)->name, parentdir, event->name);
			}
			else {
				from = create_path2( (*source_root)->name, event->name);
				to = create_path2( (*backup_root)->name, event->name);
			}
			command = create_cpcommand(from, to);
			if(system(command) == -1) perror("system cp command");; /* uses fork and execl to execute a "cp" command */

			free(from);
			free(to);
			free(command);

			if(strcmp(parentdir, (*source_root)->name) != 0) 
				backup_path = create_path2( (*backup_root)->name, parentdir);
			else {
				backup_path = (char *)malloc(strlen( (*backup_root)->name ) + 1);
				strcpy(backup_path, (*backup_root)->name);
			}

            /* update the data structures regarding the Backup directory (Backup tree, Backup inode table) */
			bino = insert_file_entity(event, backup_root, backup_inotable, backup_path, (*backup_root)->name); 

			printf("(Inside in_create): Added file entity with inode %d in Backup\n", bino);
			link_source_backup(source_inotable, sino, bino);	/* link the inode entry in Source with the inode entry in Backup */

			/* free allocated memory */
			free(parentdir);
			free(backup_path);
		}	
		/* the new file created in Source DOES HAVE a copy in backup */
		else {

			printf("Copy Exists\n");

			if(strcmp(parentdir, (*source_root)->name) != 0)  {
				to = create_path3( (*backup_root)->name, parentdir, event->name);
				backup_path = create_path2( (*backup_root)->name, parentdir);
			}
			else {
				to = create_path2( (*backup_root)->name, event->name);
				backup_path = (char *)malloc(strlen( (*backup_root)->name ) + 1);
				strcpy(backup_path, (*backup_root)->name);
			}

			printf("(Hard)link %s with %s (inode %d)\n", to, name_to_link, bino);

			/* link the file with another file in backup */
			if(link(name_to_link, to) == -1) perror("link");

            /* update the data structures regarding the Backup directory (Backup tree, Backup inode table) */
			bino = insert_file_entity(event, backup_root, backup_inotable, backup_path, (*backup_root)->name); 	
			printf("(Inside in_create): Added file entity with inode %d in Backup\n", bino);

			/* free allocated memory */
			free(parentdir);
			free(name_to_link);
			name_to_link = NULL;
			free(to);
			free(backup_path);
		}
	}
	/* in_create event was generated by a directory entity */
	else {
		char *from = NULL;

		parentdir = find_wd(*listhead, event->wd);	/* parentdir is the directory path where the directory was created */
		if(strcmp(parentdir, (*source_root)->name) != 0)
			source_path = create_path2( (*source_root)->name, parentdir );
		else {
			source_path = (char *)malloc(strlen( (*source_root)->name ) + 1);
			strcpy(source_path, (*source_root)->name);
		}

        /* update the data structures regarding the Source directory (Source tree, Source inode table) */
		int sino = insert_dir_entity(event, source_root, source_inotable, source_path, (*source_root)->name); 

		printf("(Inside in_create): Added directory entity with inode %d in Source\n", sino);
		free(source_path);

		if(strcmp(parentdir, (*source_root)->name) != 0)
			from = create_path3( (*source_root)->name, parentdir, event->name);
		else
			from = create_path2( (*source_root)->name, event->name);

        /* add a wd to the new directory */ 
		int wd = inotify_add_watch(inotifyfd, from, IN_CREATE | IN_ATTRIB | IN_MODIFY | \
								  	IN_CLOSE_WRITE | IN_DELETE | IN_DELETE_SELF \
									| IN_MOVED_FROM | IN_MOVED_TO);
		if(wd == -1) {
			perror("inotify add watch");
			exit(-1);
		}
		printf("(Inside in_create): Watching %s using wd %d\n", from, wd);
		free(from);

		if(strcmp(parentdir, (*source_root)->name) != 0)
			from = create_path2(parentdir, event->name);
		else {
			from = (char *)malloc(strlen(event->name) + 1);
			strcpy(from, event->name);
		}

        /* create and insert a new (watch_descriptor - path) pair to the wd mappings */
		watch_descriptor *newnode = create_watch_descriptor(wd, from);	
		insert_watch_descriptor(listhead, newnode);
		free(from);

		if(strcmp(parentdir, (*source_root)->name) != 0)
			to = create_path3( (*backup_root)->name, parentdir, event->name);
		else {
			to = create_path2( (*backup_root)->name, event->name);
		}
		mkdir(to, S_IRWXU | S_IRWXG | S_IRWXO); /* physically create the new directory in Backup */
		free(to);

		if(strcmp(parentdir, (*source_root)->name) != 0)
			backup_path = create_path2( (*backup_root)->name, parentdir);
		else {
			backup_path = (char *)malloc( strlen( (*backup_root)->name ) + 1);
			strcpy(backup_path, (*backup_root)->name);
		}

        /* update the data structures regarding the Backup directory (Backup tree, Backup inode table) */
		int bino = insert_dir_entity(event, backup_root, backup_inotable, backup_path, (*backup_root)->name);

		printf("(Inside in_create): Added directory entity with inode %d in Backup\n", bino);

		link_source_backup(source_inotable, sino, bino); /* link the inode entry in Source with the inode entry in Backup */

		/* free allocated memory */
		free(parentdir);
		free(backup_path);
	}
}


void in_attrib(struct inotify_event *event, watch_descriptor *listhead, node **source_root, inode_entry **source_inotable, node **backup_root, inode_entry **backup_inotable, int old_inode) {

	int rv, bino;
	char *parentdir = NULL, *source_path = NULL, *backup_path = NULL, *command = NULL;
	struct stat info;

	/* in_attrib event was generated by a file */
	if( (event->mask & IN_ISDIR) == 0) {

		parentdir = find_wd(listhead, event->wd);	/* parentdir is the directory path where the file was created */

		/* check if the event was generated from the 1st level of the source tree */
		if(strcmp(parentdir, (*source_root)->name) != 0)
			source_path = create_path3( (*source_root)->name, parentdir, event->name);
		else {
			source_path = create_path2( (*source_root)->name, event->name);
		}

		printf("(Inside in_attrib): IN_ATTRIB event was generated by %s\n", source_path);

		/* stat the file to acquire its last modification time */
		if(stat(source_path, &info) == -1) perror(source_path);		/* stat the updated file to obtain its new "last modification" date */
		rv = compare_mtimes(*source_inotable, old_inode, ctime(&info.st_mtime) );

		if(rv == 1) {	/* file's last modification time is not the same as the one in its inode entry */

            /* update the file's "last modified" date and replace its old i-node number inside its inode entry (Source) */
			bino = update_entry_date(source_inotable, old_inode, info.st_ino, ctime(&info.st_mtime));

			/* copy (physically) the updated file from Source to Backup */
			if(strcmp(parentdir, (*source_root)->name) != 0)
				backup_path = create_path3( (*backup_root)->name, parentdir, event->name);
			else 
				backup_path = create_path2( (*backup_root)->name, event->name);

			printf("(Inside in_attrib): About to copy (update) the file %s to %s\n", source_path, backup_path);
			command = create_cpcommand(source_path, backup_path);
			if(system(command) == -1) perror("system cp command");

			/* now, update the inode entry of the copy of this file in Backup */
			update_entry_date(backup_inotable, bino, bino, ctime(&info.st_mtime));

			/* free allocated memory */
			free(backup_path);
			free(command);
		}

		free(parentdir);
		free(source_path);
	}
}


int in_modify(struct inotify_event *event) {

	if( (event->mask & IN_ISDIR) == 0)
		return 1;

	return 0;
}


int in_close_write(struct inotify_event *event, watch_descriptor *listhead, node **source_root, inode_entry **source_inotable, node **backup_root, inode_entry **backup_inotable) {

	int new_source_inode, bino;
	char *parentdir = NULL, *source_path = NULL, *backup_path = NULL, *command = NULL;
	struct stat info;

	parentdir = find_wd(listhead, event->wd);		/* parentdir is the parent directory of the file that was modified */

	if(strcmp(parentdir, (*source_root)->name) != 0)
		source_path = create_path3( (*source_root)->name, parentdir, event->name);
	else 
		source_path = create_path2( (*source_root)->name, event->name);

	printf("IN_CLOSE_WRITE event was generated by %s\n", source_path);

	/* stat the file to acquire its updated size */
	if(stat(source_path, &info) == -1) perror(source_path);

	bino = update_entry_size(source_inotable, info.st_ino, info.st_size); /* update the file's size inside its inode entry (Source) */
	printf("(in_close_write): Old inode in Backup %d\n", bino);

	new_source_inode = info.st_ino;

	/* copy (physically) the updated file from Source to Backup */
	if(strcmp(parentdir, (*source_root)->name) != 0)
		backup_path = create_path3( (*backup_root)->name, parentdir, event->name);
	else 
		backup_path = create_path2( (*backup_root)->name, event->name);

	printf("(in_close_write): About to copy file %s to %s\n", source_path, backup_path);
	command = create_cpcommand(source_path, backup_path);
	if(system(command) == -1) perror("system cp command");

	/* get the new inode number of the copied file in Backup */
	if(stat(backup_path, &info) == -1) perror(backup_path);

	/* now, update the inode entry of the copy of this file in Backup */
	update_entry_size(backup_inotable, info.st_ino, info.st_size);

	link_source_backup(source_inotable, new_source_inode, info.st_ino); /* link the modified file in Source with the one in Backup */

	/* free allocated memory */
	free(parentdir);
	free(source_path);
	free(backup_path);
	free(command);

    return new_source_inode;
}


void in_delete(struct inotify_event *event, watch_descriptor *listhead, node **source_root, inode_entry **source_inotable, node **backup_root, inode_entry **backup_inotable) {

	char *parentdir = NULL, *source_path = NULL, *backup_path = NULL;

	/* if IN_DELETE event was generated by a file */
	if( (event->mask & IN_ISDIR) == 0) {

		parentdir = find_wd(listhead, event->wd);   

		if(strcmp(parentdir, (*source_root)->name) != 0)
			source_path = create_path2( (*source_root)->name, parentdir);	
		else {
			source_path = (char *)malloc(strlen( (*source_root)->name ) + 1);
			strcpy(source_path, (*source_root)->name);
		}

		printf("IN_DELETE event was generated by %s\n", source_path);

        /* function to locate and delete file's treenode and inode entry (Source) */
		delete_file_entity(source_root, source_inotable, (*source_root)->name, source_path, event->name); 

		if(strcmp(parentdir, (*source_root)->name) != 0)
			backup_path = create_path2( (*backup_root)->name, parentdir);
		else {
			backup_path = (char *)malloc(strlen( (*backup_root)->name ) + 1);
			strcpy(backup_path, (*backup_root)->name);
		}

        /* function to locate and delete file's treenode and inode entry (Backup) */
		delete_file_entity(backup_root, backup_inotable, (*backup_root)->name, backup_path, event->name);

		/* if in_delete was not called from in_moved_from */
		if( !(event->mask & IN_MOVED_FROM ) ) {
			printf("NOT FROM IN_MOVED_FROM!\n");
			char *unlink_backup_file = create_path2(backup_path, event->name);
			if(unlink(unlink_backup_file) < 0) perror("unlink file in Backup inside in_delete");
			free(unlink_backup_file);
		}

		/* free allocated memory */
		free(parentdir);
		free(source_path);
		free(backup_path);
	}
}


void in_delete_self(struct inotify_event *event, int inotifyfd, watch_descriptor *listhead, node **source_root, inode_entry **source_inotable, node **backup_root, inode_entry **backup_inotable) {

	char *parentdir = NULL, *source_path = NULL, *backup_path = NULL;

	parentdir = find_wd(listhead, event->wd);
	if(strcmp(parentdir, (*source_root)->name) != 0)
		source_path = create_path2( (*source_root)->name, parentdir);	
	else {
		source_path = (char *)malloc(strlen( (*source_root)->name ) + 1);
		strcpy(source_path, (*source_root)->name);
	}

	printf("IN_DELETE_SELF event was generated by %s\n", source_path);
	inotify_rm_watch(inotifyfd, event->wd);	/* remove watch descriptor from the monitored directory before deleting it */

    /* recursively remove a directory and all its contents */
	delete_dir_entity(source_root, source_inotable, (*source_root)->name, source_path, 0); 

	if(strcmp(parentdir, (*source_root)->name) != 0)
		backup_path = create_path2( (*backup_root)->name, parentdir);
	else {
		backup_path = (char *)malloc(strlen( (*backup_root)->name ) + 1);
		strcpy(backup_path, (*backup_root)->name);
	}

	delete_dir_entity(backup_root, backup_inotable, (*backup_root)->name, backup_path, 0);
	rmdir(backup_path);

	/* free allocated memory */
	free(parentdir);
	free(source_path);
	free(backup_path);
}


void mvfrom_unlink(watch_descriptor *listhead, char source_root_name[20], char backup_root_name[20], int wd, char event_name[35]) {

	char *parentdir = NULL, *unlink_name = NULL;

	parentdir = find_wd(listhead, wd);
	if(strcmp(parentdir, source_root_name) != 0)
		unlink_name = create_path3(backup_root_name, parentdir, event_name);
	else {
		unlink_name = create_path2(backup_root_name, event_name);
	}
	if( unlink(unlink_name) < 0 ) perror("unlink inside mvfrom_unlink");

	free(parentdir);
	free(unlink_name);
}


void in_moved_to(int mvfrom_wd, char mvfrom_name[35], struct inotify_event *event, watch_descriptor *listhead, node **source_root, inode_entry **source_inotable, node **backup_root, inode_entry **backup_inotable) {

	char *old_path = NULL, *new_path = NULL, *old_parentdir = NULL, *new_parentdir = NULL, *source_path = NULL, *backup_path = NULL;

	new_parentdir = find_wd(listhead, event->wd);	/* directory path where the file was moved to */
	old_parentdir = find_wd(listhead, mvfrom_wd);	/* directory path where the file was moved from */

	if(strcmp(new_parentdir, (*source_root)->name) != 0) {
		source_path = create_path2( (*source_root)->name, new_parentdir);
		new_path = create_path3( (*backup_root)->name, new_parentdir, event->name);
	}
	else {
		source_path = (char *)malloc(strlen( (*source_root)->name ) + 1);
		strcpy(source_path, (*source_root)->name);
		new_path = create_path2( (*backup_root)->name, event->name);
	}

	int sino = insert_file_entity(event, source_root, source_inotable, source_path, (*source_root)->name);  
	printf("(in_moved_to): Added file entity with i-node %d in Source\n", sino);

	/* move the file to its new directory (in Backup) */
	if(strcmp(old_parentdir, (*source_root)->name) != 0) {
		old_path = create_path3( (*backup_root)->name, old_parentdir, mvfrom_name);
	}
	else {
		old_path = create_path2( (*backup_root)->name, mvfrom_name);
	}

	printf("(in_moved_to): File's old path in Backup was: %s\n", old_path);	
	printf("(in_moved_to): File's new path in Backup is: %s\n", new_path);	
	if(rename(old_path, new_path) < 0) perror("rename");

	if(strcmp(new_parentdir, (*source_root)->name) != 0) 
		backup_path = create_path2( (*backup_root)->name, new_parentdir);
	else {
		backup_path = (char *)malloc(strlen( (*backup_root)->name ) + 1);
		strcpy(backup_path, (*backup_root)->name);
	}

	int bino = insert_file_entity(event, backup_root, backup_inotable, backup_path, (*backup_root)->name); 
	printf("(in_moved_to): Added file entity with inode %d in Backup\n", bino);

	link_source_backup(source_inotable, sino, bino);	/* link the inode entry in Source with the inode entry in Backup */

	/* free allocated memory */
	free(old_path);
	free(new_path);
	free(old_parentdir);
	free(new_parentdir);
	free(source_path);
	free(backup_path);
}


const char * target_type(struct inotify_event *event) {
	if( event->len == 0 )
		return "";
	else
		return event->mask & IN_ISDIR ? "directory" : "file";
}


const char * target_name(struct inotify_event *event) {
	return event->len > 0 ? event->name : NULL;
}


const char * event_name(struct inotify_event *event) {
	if (event->mask & IN_ACCESS)
		return "access";
	else if (event->mask & IN_ATTRIB)
		return "attrib";
	else if (event->mask & IN_CLOSE_WRITE)
		return "close write";
	else if (event->mask & IN_CLOSE_NOWRITE)
		return "close nowrite";
	else if (event->mask & IN_CREATE)
		return "create";
	else if (event->mask & IN_DELETE)
		return "delete";
	else if (event->mask & IN_DELETE_SELF)
		return "watch target deleted";
	else if (event->mask & IN_MODIFY)
		return "modify";
	else if (event->mask & IN_MOVE_SELF)
		return "watch target moved";
	else if (event->mask & IN_MOVED_FROM)
		return "moved out";
	else if (event->mask & IN_MOVED_TO)
		return "moved into";
	else if (event->mask & IN_OPEN)
		return "open";
	else
		return "unknown event";
}