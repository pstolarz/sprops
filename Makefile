YACC=bison
CC=gcc
CFLAGS=-Wall -I./inc
MAKEDEP=gcc $(CFLAGS) $< -MM -MT "$@ $*.o" -MF $@

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
	rm -f libsprops.a $(OBJS) $(OBJS:.o=.d)
	$(MAKE) -C./ut clean

ut_run:
	make -C./ut run

%.c: %.y
	$(YACC) $< -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

libsprops.a: $(OBJS)
	ar rcs $@ $(OBJS)

%.d: %.c
		$(MAKEDEP)

ifneq ($(MAKECMDGOALS),clean)
    -include $(OBJS:.o=.d)
endif
