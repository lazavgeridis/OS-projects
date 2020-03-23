/* inotify_events.h */

#include "DirectoryTreeStructures.h"

void in_create(struct inotify_event *, int, watch_descriptor **, node **, inode_entry **, node **, inode_entry **);
void in_attrib(struct inotify_event *, watch_descriptor *, node **, inode_entry **, node **, inode_entry **, int);
int in_close_write(struct inotify_event *, watch_descriptor *, node **, inode_entry **, node **, inode_entry **);
void in_delete(struct inotify_event *, watch_descriptor *, node **, inode_entry **, node **, inode_entry **);
void in_delete_self(struct inotify_event *, int, watch_descriptor *, node **, inode_entry **, node **, inode_entry **);
void mvfrom_unlink(watch_descriptor *, char [20], char [20], int, char [35]);
void in_moved_to(int, char [35], struct inotify_event *, watch_descriptor *, node **, inode_entry **, node **, inode_entry **);
int in_modify(struct inotify_event *);

const char * target_type(struct inotify_event *event);
const char * target_name(struct inotify_event *event);
const char * event_name(struct inotify_event *event);
