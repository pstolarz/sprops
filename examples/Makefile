CC = $(CROSS_COMPILE)gcc
CFLAGS += -Wall -I../inc

EXAMPLES = \
    basic \
    store \
    store-upd

.PHONY: all clean FORCE

all: $(EXAMPLES)

clean:
	$(RM) $(EXAMPLES)

../libsprops.a: FORCE
	$(MAKE) -C..

%: %.c ../libsprops.a
	$(CC) $(CFLAGS) $^ -o $@
