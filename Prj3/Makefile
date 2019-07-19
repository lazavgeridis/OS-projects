
CC = gcc
CFLAGS = -O -Wall -g

all: myport vessel port_master monitor

myport: myport.o myportImplementation.o 
	$(CC) -pthread -o myport myport.o myportImplementation.o $(CFLAGS)

myport.o: myport.c myportInterface.h SharedMem.h
	$(CC) -c myport.c $(CFLAGS)

myportImplementation.o:	myportImplementation.c 
	$(CC) -c myportImplementation.c $(CFLAGS)

vessel: vessel.c SharedMem.h
	$(CC) -pthread -o vessel vessel.c $(CFLAGS)

port_master: port_master.c SharedMem.h
	$(CC) -pthread -o port_master port_master.c $(CFLAGS)

monitor: monitor.c SharedMem.h
	$(CC) -pthread -o monitor monitor.c $(CFLAGS)



clean:
	rm myport vessel port_master monitor myport.o myportImplementation.o
