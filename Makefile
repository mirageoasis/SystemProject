CC = gcc
CFLAGS = -g
LDLIBS = -lpthread

PROGS = shellex

all: $(PROGS)

shellex: shellex.c csapp.c mycd.c job.c signal_handler.c

clean:
	rm -rf *~ $(PROGS)

