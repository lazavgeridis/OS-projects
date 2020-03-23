/* DirectoryTreeInterface.h */

#include <sys/types.h>
#include "DirectoryTreeStructures.h"

void sync_directories(node **, node **, inode_entry **, inode_entry **, char *, char *);
void add_watch_descriptors(watch_descriptor **, int, node *, char *, char *);
void remove_watch_descriptors(watch_descriptor **, int);
void construct_tree(inode_entry **, char *, node **);
void sort_siblings(node **);
void cleanup(node **, node **, inode_entry **, inode_entry **, watch_descriptor **, int);

int get_inode(struct inotify_event *, watch_descriptor *, char [20], char [35]);
node *create_node(char *, int);

/* for testing */
void print_directory_contents(node *);
void print_inotable(inode_entry *);
