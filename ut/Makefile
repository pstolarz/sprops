CC=gcc
CFLAGS=-Wall -I../inc

.PHONY: all clean

TESTS=spget spset_mit

all: $(TESTS)

clean:
	rm -f $(TESTS)

%: %.c ../libsprops.a
	$(CC) $(CFLAGS) $^ -o $@

../libsprops.a:
	$(MAKE) -C..
