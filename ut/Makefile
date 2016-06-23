CC=gcc
CFLAGS=-Wall -I../inc

.PHONY: all clean makelib

TESTS= \
    t01-get \
    t02-iter \
    t03-tknstr \
    t04-add \
    t05-rm \
    t06-set

all: $(TESTS)

clean:
	rm -f $(TESTS)

makelib ../libsprops.a:
	$(MAKE) -C..

%: %.c makelib ../libsprops.a
	@if [ "$?" != "makelib" ]; then \
	  (PS4=; set -x; $(CC) $(CFLAGS) $< -o $@ ../libsprops.a;) \
	fi
