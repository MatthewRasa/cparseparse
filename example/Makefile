CC := g++
CPPFLAGS := -I../include
CXXFLAGS := -Wall -Wextra -std=c++11 -O3

BDIR := bin
CDIR := src

$(BDIR)/sort-string: $(CDIR)/sort-string.cc
	@mkdir -p $(shell dirname $@)
	$(CC) -o $@ $(CPPFLAGS) $(CXXFLAGS) $^

clean:
	@rm -rvf $(BDIR)
