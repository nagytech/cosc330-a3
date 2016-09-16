CC=mpicc

CFLAGS=-Wall -pedantic -std=c99 -D _BSD_SOURCE
LIBS=-lm

OBJECTS=gaussianLib.o init.o kern.o master.o mosaic.o qdbmp.o slave.o

all: gaussianmpi $(OBJECTS)

debug: CFLAGS += -DDEBUG -g
debug: all

gaussianmpi: $(OBJECTS) gaussianmpi.c
	$(CC) $(CFLAGS) $(OBJECTS) gaussianmpi.c -o gaussianmpi $(LIBS)

clean:
	rm -f gaussianmpi $(OBJECTS)
