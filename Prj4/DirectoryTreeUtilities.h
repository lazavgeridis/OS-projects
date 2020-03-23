/* DirectoryTreeUtilities.h */

#include <sys/types.h>
#include "DirectoryTreeStructures.h"

int insert_file_entity(struct inotify_event *, node **, inode_entry **, char *, char *);
int insert_dir_entity(struct inotify_event *, node **, inode_entry **, char *, char *);
int delete_file_entity(node **, inode_entry **, char *, char *, char *);
int delete_dir_entity(node **, inode_entry **, char *, char *, int);
int update_entry_date(inode_entry **, int, int, char*);
int update_entry_size(inode_entry **, int, int);
int copy_exists(inode_entry *, inode_entry *, int, char **);
int inode_exists(inode_entry *, int);
int delete_node(node **, char *, char);
int compare_mtimes(inode_entry *, int, char *);

void insert_watch_descriptor(watch_descriptor **, watch_descriptor *);
void insert_entry(inode_entry **, char *, int, char *, off_t);
void delete_entry(inode_entry **, char *, int);
void delete_file_list(node **, inode_entry **, char *, int);
void remove_unwanted_dirs(node **, inode_entry **, char *);
void remove_unwanted_files(node **, inode_entry **, char *);
void remove_directory(node **, inode_entry **, char *, int);
void link_source_backup(inode_entry **, int, int);
void filename_eq_dirname(node **, inode_entry **, char *, char *);

char *find_wd(watch_descriptor *, int);
char *create_path2(char *, char *);
char *create_path3(char *, char *, char *);
char *create_cpcommand(char *, char *);

name_node *create_name_node(char *);
inode_entry *create_inode_entry(int, char *, off_t);
watch_descriptor *create_watch_descriptor(int, char *);
