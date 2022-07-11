all: test

test:	test.cpp Makefile eNET-types.h TMessage.cpp TMessage.h
	g++ -g -Wfatal-errors -std=gnu++2a -o test test.cpp TMessage.cpp -lm -lpthread -O3

clean:
	rm -f test