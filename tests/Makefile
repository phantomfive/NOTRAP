CC = gcc

CFLAGS = -IcuTest -I../publicHeaders -ggdb -O1 -Wall -Werror
LDFLAGS = 

CSRC = $(wildcard *.c) cuTest/CuTest.c $(wildcard ../src/*.c)
HDRS = $(wildcard *.h) cuTest/CuTest.h $(wildcard ../src/*.h)

run: notrapTests
	./notrapTests

notrapTests: $(CSRC) $(HDRS)
	$(CC) -o notrapTests $(CFLAGS) $(CSRC) $(LDFLAGS)

clean:
	rm -fr notrapTests notrapTests.dSYM

.PHONY: run clean
