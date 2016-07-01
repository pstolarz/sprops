YACC=bison
CC=gcc
CFLAGS=-Wall -I./inc

# use alloca() instead of malloc() for the grammar parser stack allocations
CFLAGS+=-DYYSTACK_USE_ALLOCA

OBJS = \
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

config.h=config.h
./inc/sprops/props.h=./inc/sprops/props.h
./inc/sprops/parser.h=./inc/sprops/parser.h $(./inc/sprops/props.h)

parser.o: $(./inc/sprops/parser.h)
props.o: $(./inc/sprops/props.h)

parser.c: parser.y
	$(YACC) $< -o $@

%.o: %.c $(config.h)
	$(CC) -c $(CFLAGS) $< -o $@

libsprops.a: $(OBJS)
	ar rcs $@ $(OBJS)
