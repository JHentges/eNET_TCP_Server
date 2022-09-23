all: test aioenetd

#GCC doesn't include libstdc++, so just override it here
GCC := aarch64-linux-g++

test:	test.cpp Makefile eNET-types.h TMessage.cpp TMessage.h TError.h TError.cpp apcilib.cpp logging.h adc.cpp adc.h
	$(GCC) -g -Wfatal-errors -std=gnu++2a -o test test.cpp TError.cpp logging.cpp TMessage.cpp apcilib.cpp adc.cpp -lm -lpthread -latomic -O3

aioenetd:	aioenetd.cpp Makefile eNET-types.h TMessage.cpp TMessage.h TError.h TError.cpp apcilib.cpp adc.cpp adc.h logging.h logging.cpp
	$(GCC) -g -Wfatal-errors -std=gnu++2a -o aioenetd aioenetd.cpp TError.cpp logging.cpp TMessage.cpp apcilib.cpp adc.cpp -lm -lpthread -latomic -O3

clean:
	rm -f test aioenetd
