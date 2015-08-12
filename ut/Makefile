CC=gcc
CFLAGS=-Wall -I../inc

.PHONY: all clean

all: spget

clean:
	rm -f spget

spget: spget.c ../libsprops.a
	$(CC) $(CFLAGS) $^ -o $@

../libsprops.a:
	$(MAKE) -C..
