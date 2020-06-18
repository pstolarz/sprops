.PHONY: all clean

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

all: libsprops.a

clean:
	$(RM) libsprops.a $(OBJS) $(OBJS:.o=.d)

libsprops.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

%.c: %.y
	$(YACC) $< -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.d: %.c
	$(MAKEDEP)

ifneq ($(MAKECMDGOALS),clean)
-include $(OBJS:.o=.d)
endif
