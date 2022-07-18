all: test

test:	test.cpp Makefile eNET-types.h TMessage.cpp TMessage.h TError.h TError.cpp apcilib.cpp
	g++ -g -Wfatal-errors -std=gnu++2a -o test test.cpp TError.cpp TMessage.cpp apcilib.cpp -lm -lpthread -O3

clean:
	rm -f test