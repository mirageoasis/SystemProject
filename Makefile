CC = gcc
CFLAGS = -Og
LDLIBS = -lpthread

PROGS = myshell

all: $(PROGS)

myshell: myshell.c csapp.c mycd.c job.c

clean:
	rm -rf *~ $(PROGS)

