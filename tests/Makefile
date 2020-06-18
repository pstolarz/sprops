.PHONY: all clean libsprops test

LIBSPROPS_DIR=../src
CC = $(CROSS_COMPILE)gcc
CFLAGS += -Wall -I$(LIBSPROPS_DIR)/inc

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

all: libsprops test

clean:
	$(RM) $(TESTS)

libsprops:
	$(MAKE) -C$(LIBSPROPS_DIR)

test: $(TESTS)
	@chk_diff() { \
	  echo -n "$$1: "; \
	  if [ `./$$1 | diff - ./$$2 | wc -c` -gt 0 ]; \
	  then \
	    echo "FAILED: Differences found between test's output and $$2"; \
	  else \
	    echo "OK"; \
	  fi; \
	}; \
	chk_diff t01-get t01.out; \
	chk_diff t02-iter t02.out; \
	./t03-tknstr 2>&1 1>/dev/null; \
	if [ $$? -eq 0 ]; \
	then \
	  echo "t03-tknstr: OK"; \
	else \
	  echo "t03-tknstr: FAILED"; \
	fi; \
	chk_diff t04-add t04.out; \
	chk_diff t05-rm t05.out; \
	chk_diff t06-set t06.out; \
	chk_diff t07-mv t07.out; \
	chk_diff t08-scratch t08.out; \
	chk_diff t09-trans t09.out;

%: %.c
	$(CC) $(CFLAGS) $< -o $@ -L$(LIBSPROPS_DIR) -lsprops
