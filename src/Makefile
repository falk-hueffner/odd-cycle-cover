CC	= gcc
CFLAGS	= -std=c99 -O3 -march=native -g -W -Wall -pipe # -DNDEBUG
# disable internal consistency checking for moderate (10-30%) speedup
#CFLAGS  += -DNDEBUG

PROG	= occ

LDPATH	=
LIBS	=

SOURCES	= \
	util.c		\
	bitvec.c	\
	edge-flow.c	\
	edge-occ.c	\
	flow.c		\
	graph.c		\
	hash-table.c	\
	main.c		\
	occ.c		\
	occ-enum2col.c	\
	occ-gray.c	\
	occ-heuristic.c


CCOMPILE = $(CC) $(CFLAGS)
CLINK	= $(CC) $(CFLAGS) $(LDPATH) $(LIBS)

OBJS	= $(SOURCES:.c=.o)

all: depend $(PROG)

%.o: %.c
	$(CCOMPILE) -c $<

$(PROG): $(OBJS)
	$(CLINK) $(OBJS) -o $(PROG)

clean:
	rm -f $(PROG) $(OBJS) core gmon.out

realclean: clean
	rm -f *~ *.bak

.depend: depend

depend: $(SOURCES)
	$(CC) $(CFLAGS) -MM *.c > .depend

include .depend
