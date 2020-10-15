# 
# Author: Matthew Rasa
# E-mail: matt@raztech.com
# GitHub: https://github.com/MatthewRasa
#

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
