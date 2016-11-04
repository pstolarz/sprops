CC = $(CROSS_COMPILE)gcc
CFLAGS += -Wall -I./inc
# use alloca() instead of malloc() for the grammar parser stack allocations
CFLAGS += -DYYSTACK_USE_ALLOCA

AR = $(CROSS_COMPILE)ar
YACC = bison
MAKEDEP = $(CROSS_COMPILE)gcc $(CFLAGS) $< -MM -MT "$@ $*.o" -MF $@

OBJS = \
    io.o \
    utils.o \
    parser.o \
    props.o \
    trans.o

.PHONY: all clean examples ut_run tags

all: libsprops.a

clean:
	$(RM) libsprops.a $(OBJS) $(OBJS:.o=.d) tags
	$(MAKE) -C./examples clean
	$(MAKE) -C./ut clean

examples:
	$(MAKE) -C./examples

ut_run:
	$(MAKE) -C./ut run

tags:
	ctags -R --c-kinds=+px --c++-kinds=+px .

libsprops.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

%.c: %.y
	$(YACC) $< -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.d: %.c
	$(MAKEDEP)

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),tags)
-include $(OBJS:.o=.d)
endif
endif
