# 
# Author: Matthew Rasa
# E-mail: matt@raztech.com
# GitHub: https://github.com/MatthewRasa
#

ODIR := obj
CDIR := src
BDIR := bin

include ../common.mk

$(BDIR)/sort-string: $(OLIST)
	@mkdir -p $(shell dirname $@)
	$(CC) -o $@ $(CPPFLAGS) $(CXXFLAGS) $^

clean:
	@rm -rvf $(BDIR) $(ODIR)
