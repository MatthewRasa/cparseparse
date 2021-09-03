# 
# Author: Matthew Rasa
# E-mail: matt@raztech.com
# GitHub: https://github.com/MatthewRasa
#

# Build options
BUILD := release
STD := c++11

CC := g++
CPPFLAGS := -I../include
CXXFLAGS := -Wall -Wextra -std=$(STD)

ifeq ($(BUILD),debug)
CXXFLAGS += -O0 -g
else ifeq ($(BUILD),release)
CXXFLAGS += -O3
else
$(error BUILD must be either 'debug' or 'release')
endif

ifeq ($(CDIR),)
$(error CDIR must be defined)
endif

ifeq ($(ODIR),)
$(error ODIR must be defined)
endif

CLIST := $(shell find $(CDIR) -name "*.cc")
OLIST := $(CLIST:$(CDIR)/%.cc=$(ODIR)/%.o)

$(ODIR)/%.o: $(CDIR)/%.cc
	@mkdir -p $(shell dirname $@)
	$(CC) -o $@ -c $(CPPFLAGS) $(CXXFLAGS) $^
