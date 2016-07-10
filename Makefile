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

config.h=config.h
inc/sprops/props.h=inc/sprops/props.h
io.h=io.h $(inc/sprops/props.h)
inc/sprops/utils.h=inc/sprops/utils.h $(inc/sprops/props.h)
inc/sprops/parser.h=inc/sprops/parser.h $(inc/sprops/props.h)
inc/sprops/trans.h=inc/sprops/trans.h $(inc/sprops/props.h)

io.o: $(io.h)
utils.o: $(inc/sprops/utils.h)
parser.o: $(inc/sprops/parser.h)
props.o: $(inc/sprops/parser.h) $(inc/sprops/utils.h)
trans.o: $(inc/sprops/trans.h) $(inc/sprops/utils.h)

parser.c: parser.y
	$(YACC) $< -o $@

%.o: %.c $(config.h) $(io.h)
	$(CC) -c $(CFLAGS) $< -o $@

libsprops.a: $(OBJS)
	ar rcs $@ $(OBJS)
