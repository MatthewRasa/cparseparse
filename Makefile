# 
# Author: Matthew Rasa
# E-mail: matt@raztech.com
# GitHub: https://github.com/MatthewRasa
#

PREFIX := /usr/local

.PHONY: all test example clean-test clean-example clean

all: test example

test:
	$(MAKE) -C test

example:
	$(MAKE) -C example

clean-test:
	$(MAKE) -C test clean

clean-example:
	$(MAKE) -C example clean

clean: clean-test clean-example

install:
	@for path in $(shell find include/cparseparse -type f); do \
		install -v -D $$path $(PREFIX)/$$path; \
	done

uninstall:
	@rm -rvf $(PREFIX)/include/cparseparse

run-tests: test
	./test/unit-tests

run-valgrind-tests: test
	valgrind ./test/unit-tests
