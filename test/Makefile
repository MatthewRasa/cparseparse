# 
# Author: Matthew Rasa
# E-mail: matt@raztech.com
# GitHub: https://github.com/MatthewRasa
#

CDIR := src
ODIR := obj
APPNAME := unit-tests

include ../common.mk

$(APPNAME): $(OLIST)
	$(CC) $^ -o $@

clean:
	@rm -rvf $(ODIR) $(APPNAME)
