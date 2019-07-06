CC = gcc
CFLAGS = -Wall -g

all:	myfind splitter_merger searcher

myfind:	root.c
	$(CC) -o myfind root.c $(CFLAGS)

splitter_merger: splitter_merger.o sm_utils.o
	$(CC) -o splitter_merger splitter_merger.o sm_utils.o $(CFLAGS)

splitter_merger.o: splitter_merger.c sm_utils.h
	$(CC) -c splitter_merger.c $(CFLAGS)

sm_utils.o: sm_utils.c
	$(CC) -c sm_utils.c $(CFLAGS)

searcher: searcher.c
	$(CC) -o searcher searcher.c $(CFLAGS)

clean:
	rm 	myfind splitter_merger searcher splitter_merger.o sm_utils.o
