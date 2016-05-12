CC=gcc
CFLAGS=-Wall -I../inc

.PHONY: all clean

TESTS= \
    t01-get \
    t02-iter

all: $(TESTS)

clean:
	rm -f $(TESTS)

%: %.c ../libsprops.a
	$(CC) $(CFLAGS) $^ -o $@

../libsprops.a:
	$(MAKE) -C..
