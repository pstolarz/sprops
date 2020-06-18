.PHONY: all clean libsprops

LIBSPROPS_DIR=../src
CC = $(CROSS_COMPILE)gcc
CFLAGS += -Wall -I$(LIBSPROPS_DIR)/inc

EXAMPLES = \
    basic \
    store \
    store-upd

all: libsprops $(EXAMPLES)

clean:
	$(RM) $(EXAMPLES)

libsprops:
	$(MAKE) -C$(LIBSPROPS_DIR)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ -L$(LIBSPROPS_DIR) -lsprops
