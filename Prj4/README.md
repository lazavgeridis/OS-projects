## Description
This is the final project of the Operating Systems course. In this project,  I developed the app *mirr*, an app that performs dynamic monitoring on an hierarchy of files and directories. When a modification occurs, the app updates the exact copy of the monitored hierarchy located in a backup folder. For this purpose, the Linux *inotify* system call interface is used.  

The inode (index node) is a data structure in the Linux operating system that describes a file-system object such as a file or a directory. Each inode is linked to the data of one and only file and stores all the information related to that file, except its filename (i.e i-node number, last modification date, size, etc). Therefore, each file is linked to a unique i-node, but an i-node can be linked to multiple filenames. This is achieved via hard links. In general, the logical structure of the Linux filesystem can be outlined by an acyclic directed graph (or a tree).

![Screenshot](hierarchy.png)  

During the initial synchronization, the directory structure of the **source** directory gets reflected on the specified **backup** directory. At the end of this step, the 2 hierarchies will be identical.  
At the second step, we initiate the monitoring of the directories under the source directory (including source). The inotify events of interest are the following:  
1. **IN_CREATE** (create a file or a directory)
2. **IN_ATTRIB** (file's last date of modification has changed)
3. **IN_MODIFY** (file was modified)
4. **IN_CLOSE_WRITE** (file was written)
5. **IN_DELETE** (file was deleted - *rm* command)
6. **IN_DELETE_SELF** (directory was deleted - *rmdir* command)
7. **IN_MOVED_FROM** (file was moved outside of the monitored catalog - either *mv* command or delete from keyboard)
8. **IN_MOVED_TO** (file was moved/introduced within the monitored hierarchy)  

Each of the above-mentioned events will be reflected on the backup directory as well.

## Execution 

```
$ make  
$ ./mirr <source_directory> <backup_directory> 

```  


