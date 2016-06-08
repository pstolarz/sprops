CC=gcc
CFLAGS=-Wall -I../inc

.PHONY: all clean

TESTS= \
    t01-get \
    t02-iter \
    t03-tknstr \
    t04-add

all: $(TESTS)

clean:
	rm -f $(TESTS)

../config.h=../config.h
t03-tknstr: $(../config.h)

%: %.c ../libsprops.a
	$(CC) $(CFLAGS) $^ -o $@

../libsprops.a:
	$(MAKE) -C..
