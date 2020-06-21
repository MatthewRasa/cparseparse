CC := g++
CPPFLAGS := -I../include
CXXFLAGS := -Wall -Wextra -std=c++11 -O3

APPNAME := unit-tests
CDIR := src
ODIR := obj

CLIST := $(shell find $(CDIR) -name "*.cc")
OLIST := $(CLIST:$(CDIR)/%.cc=$(ODIR)/%.o)

$(APPNAME): $(OLIST)
	$(CC) $^ -o $@

$(ODIR)/%.o: $(CDIR)/%.cc
	@mkdir -p $(shell dirname $@)
	$(CC) $(CPPFLAGS) $(CXXFLAGS) -c $^ -o $@

clean:
	@rm -rvf $(ODIR) $(APPNAME)
