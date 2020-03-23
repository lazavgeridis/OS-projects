/* Directory Tree Structures */

#ifndef DIRECTORYTREE_STRUCTURES_H
#define DIRECTORYTREE_STRUCTURES_H

#define EVENT_SIZE 	( sizeof(struct inotify_event) )
#define EVENT_BUF_LEN	( 1024 * (EVENT_SIZE + 16) ) 


typedef struct TreeNode node;

struct TreeNode {

	char name[35];
	int  inode;     /* a treenode entity can either be a file or a directory */
	int  file_children;
	int  dir_children;
	node *list_of_dir_children;
	node *list_of_file_children;

	node *next_dir_sibling;
    node *prev_dir_sibling;

	node *next_file_sibling;
	node *prev_file_sibling;

};

typedef struct NameNode name_node;

struct NameNode{

	char *entity_name;
	name_node *next;
};

typedef struct InodeEntry inode_entry;

struct InodeEntry{

	int inode_number;
	char last_modified[28];
	int backup_inode;
	unsigned int size;
	unsigned int num_of_linked_files;   /* an i-node can be linked to multiple filenames */
	name_node *head;                    /* list of filenames a particular i-node is linked to */
	inode_entry *next;

};

typedef struct WatchDescriptor watch_descriptor;

struct WatchDescriptor{
	
	int wd;
	char *pathname;
	watch_descriptor *next;
};

#endif
