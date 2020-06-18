.PHONY: all clean libsprops examples test

all: libsprops

clean:
	$(MAKE) -C./src clean
	$(MAKE) -C./examples clean
	$(MAKE) -C./tests clean

libsprops:
	$(MAKE) -C./src

examples:
	$(MAKE) -C./examples

test:
	$(MAKE) -C./tests
