CC = gcc
CFLAGS = -Wall -g

all: mygraph

mygraph: mygraph_main.o PromptImplementation.o Utilities.o
	$(CC) -o mygraph mygraph_main.o PromptImplementation.o Utilities.o $(CFLAGS)


mygraph_main.o:	mygraph_main.c PromptInterface.h Utilities.h 
	$(CC) -c mygraph_main.c $(CFLAGS)


PromptImplementation.o: PromptImplementation.c Utilities.h 
	$(CC) -c PromptImplementation.c $(CFLAGS)

Utilities.o: Utilities.c GraphTypes.h
	$(CC) -c Utilities.c $(CFLAGS)


clean:
	rm mygraph mygraph_main.o PromptImplementation.o Utilities.o
