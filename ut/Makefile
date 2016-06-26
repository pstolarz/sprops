CC=gcc
CFLAGS=-Wall -I../inc

.PHONY: all clean makelib run

TESTS= \
    t01-get \
    t02-iter \
    t03-tknstr \
    t04-add \
    t05-rm \
    t06-set \
    t07-mv

all: $(TESTS)

clean:
	rm -f $(TESTS)

makelib ../libsprops.a:
	$(MAKE) -C..

run: all
	./ut-run.sh

%: %.c makelib ../libsprops.a
	@if [ "$?" != "makelib" ]; then \
	  (PS4=; set -x; $(CC) $(CFLAGS) $< -o $@ ../libsprops.a;) \
	fi
