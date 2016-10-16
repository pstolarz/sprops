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

.PHONY: all clean ut_run ctags

all: libsprops.a

clean:
	rm -f libsprops.a $(OBJS) $(OBJS:.o=.d) tags
	$(MAKE) -C./ut clean

ut_run:
	make -C./ut run

ctags:
	ctags -R --c-kinds=+px --c++-kinds=+px .

libsprops.a: $(OBJS)
	ar rcs $@ $(OBJS)

%.c: %.y
	$(YACC) $< -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.d: %.c
	$(MAKEDEP)

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),ctags)
-include $(OBJS:.o=.d)
endif
endif
