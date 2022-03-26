CC = gcc
CFLAGS = -Og
LDLIBS = -lpthread

PROGS = shellex

all: $(PROGS)

shellex: shellex.c csapp.c mycd.c

clean:
	rm -rf *~ $(PROGS)

