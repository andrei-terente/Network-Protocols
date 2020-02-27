CC=gcc
CFLAFS=-O0 -g -std=c89 -pedantic-errors


CPPC=g++
CPPFLAGS=-O0 -g -std=c++11 -w

all: client

.PHONY: client
client: client.cpp helpers.c requests.cpp parson.c
	$(CPPC) $(CPPFLAGS) -o $@ $^
	./$@

clean: 
	rm -f client *.o
