PROG	= occ
CC	= gcc-3.4
#CFLAGS	= -std=c99 -funroll-all-loops -O3 $(shell gcc-arch) -g -W -Wall -pipe #  -DNDEBUG
CFLAGS	= -std=c99 -O2 -march=k8 -g -W -Wall -pipe -DNDEBUG

LDPATH	=
LIBS	=

SOURCES	= \
	bitvec.c	\
	flow.c		\
	graph.c		\
	occ.c		\
	main.c


CCOMPILE = $(CC) $(CFLAGS)
CLINK	= $(CC) $(CFLAGS) $(LDPATH) $(LIBS)

OBJS	= $(SOURCES:.c=.o)

all: depend $(PROG)

%.o: %.c
	$(CCOMPILE) -c $<

$(PROG): $(OBJS)
	$(CLINK) $(OBJS) -o $(PROG)

clean:
	rm -f $(PROG) $(OBJS) core

realclean: clean
	rm -f *~ *.bak

.depend: depend

depend: $(SOURCES)
	$(CC) $(CFLAGS) -MM *.c > .depend

include .depend
