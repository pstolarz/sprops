CC=gcc
CFLAGS=-Wall -I../inc

.PHONY: all clean

all: sptest

clean:
	rm -f sptest

sptest: sptest.c ../libsprops.a
	$(CC) $(CFLAGS) $^ -o $@

../libsprops.a:
	$(MAKE) -C..
