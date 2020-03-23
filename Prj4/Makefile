
CC = gcc
CFLAGS = -O -Wall -g


all: mirr

mirr: directory_tree.o DirectoryTreeImplementation.o DirectoryTreeUtilities.o inotify_events.o
		$(CC) -o mirr directory_tree.o DirectoryTreeImplementation.o DirectoryTreeUtilities.o inotify_events.o $(CFLAGS)

directory_tree.o: directory_tree.c DirectoryTreeStructures.h DirectoryTreeInterface.h
		$(CC) -c directory_tree.c $(CFLAGS)

DirectoryTreeImplementation.o: DirectoryTreeImplementation.c DirectoryTreeStructures.h DirectoryTreeUtilities.h
		$(CC) -c DirectoryTreeImplementation.c $(CFLAGS)

DirectoryTreeUtilities.o: DirectoryTreeUtilities.c DirectoryTreeStructures.h
		$(CC) -c DirectoryTreeUtilities.c $(CFLAGS)

inotify_events.o: inotify_events.c DirectoryTreeUtilities.h
		$(CC) -c inotify_events.c $(CFLAGS)


clean:
	rm mirr directory_tree.o DirectoryTreeImplementation.o DirectoryTreeUtilities.o inotify_events.o
