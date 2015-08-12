YACC=bison
CC=gcc
CFLAGS=-Wall -I./inc

OBJS = \
    parser.o \
    props.o

.PHONY: all clean

all: libsprops.a

clean:
	rm -f libsprops.a $(OBJS)
	$(MAKE) -C./ut clean

config.h=config.h
./inc/sp_props/props.h=./inc/sp_props/props.h
./inc/sp_props/parser.h=./inc/sp_props/parser.h $(./inc/sp_props/props.h)

parser.o: $(./inc/sp_props/parser.h)
props.o: $(./inc/sp_props/props.h)

parser.c: parser.y
	$(YACC) $< -o $@

%.o: %.c $(config.h)
	$(CC) -c $(CFLAGS) $< -o $@

libsprops.a: $(OBJS)
	ar rcs $@ $(OBJS)
