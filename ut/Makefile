CC=gcc
CFLAGS=-Wall -I../inc

.PHONY: all clean run FORCE

TESTS= \
    t01-get \
    t02-iter \
    t03-tknstr \
    t04-add \
    t05-rm \
    t06-set \
    t07-mv \
    t08-scratch \
    t09-trans

all: $(TESTS)

clean:
	rm -f $(TESTS)

../libsprops.a: FORCE
	$(MAKE) -C..

run: all
	./ut-run.sh

%: %.c ../libsprops.a
	$(CC) $(CFLAGS) $^ -o $@
