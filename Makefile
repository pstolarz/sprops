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

./inc/sp_props/props.h=./inc/sp_props/props.h
./inc/sp_props/parser.h=./inc/sp_props/parser.h $(./inc/sp_props/props.h)

parser.o: $(./inc/sp_props/parser.h)
props.o: $(./inc/sp_props/props.h)

parser.c: parser.y $(./inc/sp_props/parser.h)
	$(YACC) $< -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

libsprops.a: $(OBJS)
	ar rcs $@ $(OBJS)
