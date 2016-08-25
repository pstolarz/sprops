YACC=bison
CC=gcc
CFLAGS=-Wall -I./inc

# use alloca() instead of malloc() for the grammar parser stack allocations
CFLAGS+=-DYYSTACK_USE_ALLOCA

OBJS = \
    io.o \
    utils.o \
    parser.o \
    props.o \
    trans.o

.PHONY: all clean ut_run

all: libsprops.a

clean:
	rm -f libsprops.a $(OBJS)
	$(MAKE) -C./ut clean

ut_run:
	make -C./ut run

# headers dependencies
io.h: inc/sprops/props.h
inc/sprops/parser.h: inc/sprops/props.h
inc/sprops/trans.h: inc/sprops/props.h
inc/sprops/utils.h: inc/sprops/props.h

%.h:
	@touch -c $@

io.o: io.h
parser.o: config.h io.h inc/sprops/parser.h
props.o: config.h io.h inc/sprops/parser.h inc/sprops/utils.h
trans.o: config.h io.h inc/sprops/trans.h inc/sprops/utils.h
utils.o: config.h io.h inc/sprops/utils.h

%.c: %.y
	$(YACC) $< -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

libsprops.a: $(OBJS)
	ar rcs $@ $(OBJS)
