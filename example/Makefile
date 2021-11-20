# 
# Author: Matthew Rasa
# E-mail: matt@raztech.com
# GitHub: https://github.com/MatthewRasa
#

ODIR := obj
CDIR := src
APP := sort-string

include ../common.mk

$(APP): $(OLIST)
	@mkdir -p $(shell dirname $@)
	$(CC) -o $@ $(CPPFLAGS) $(CXXFLAGS) $^

clean:
	@rm -rvf $(APP) $(ODIR)
