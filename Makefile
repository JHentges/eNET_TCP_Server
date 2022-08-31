all: test aioenetd

test:	test.cpp Makefile eNET-types.h TMessage.cpp TMessage.h TError.h TError.cpp logging.h logging.cpp apcilib.cpp safe_queue.h adc.cpp adc.h
	g++ -g -Wfatal-errors -std=gnu++2a -o test test.cpp TError.cpp logging.cpp TMessage.cpp apcilib.cpp adc.cpp -lm -lpthread -latomic -O3

aioenetd:	Makefile eNET-types.h TMessage.cpp TMessage.h TError.h TError.cpp logging.h logging.cpp aioenetd.cpp apcilib.cpp safe_queue.h adc.cpp adc.h
	g++ -g -Wfatal-errors -std=gnu++2a -o aioenetd aioenetd.cpp TError.cpp logging.cpp TMessage.cpp apcilib.cpp adc.cpp -lm -lpthread -latomic -O3

clean:
	rm -f test aioenetd