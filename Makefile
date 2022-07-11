all: test

test:	test.cpp eNET-types.h TMessage.cpp TMessage.h
	g++ -g -std=gnu++2a -o test test.cpp TMessage.cpp -lm -lpthread 

clean:
	rm -f test