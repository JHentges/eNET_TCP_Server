#.PHONY: all
#.PHONY: clean
#all: aioenetd

#GCC doesn't include libstdc++, so just override it here
#FLAGS := -g -Wfatal-errors -std=gnu++2a
#SRCS := $(wildcard ./*.cpp) $(wildcard ./DataItems/*.cpp)
#INCS := $(wildcard ./*.h) $(wildcard ./DataItems/*.h)
#ARCH:=$(shell arch)
#GCC := g++
#ifeq ($(ARCH),aarch64)
#	GCC := aarch64-linux-g++
#endif
#OBJS:=$(SRCS:.c=.o)


#%.o: %.cpp
#	$(GCC) -Wfatal-errors -std=gnu++2a -C $@ -o $< $(FLAGS)

#test:	test.cpp Makefile eNET-types.h TMessage.cpp TMessage.h TError.h TError.cpp apcilib.cpp logging.h adc.cpp adc.h
#	$(GCC) $(FLAGS) -o test test.cpp TError.cpp logging.cpp TMessage.cpp apcilib.cpp adc.cpp -lm -lpthread -latomic -O3

#aioenetd:	Makefile $(wildcard *.h) $(wildcard *.cpp) $(wildcard DataItems/*.cpp) $(wildcard DataItems/*.h)
#	$(GCC) -g -Wfatal-errors -std=gnu++2a -o aioenetd $(wildcard *.cpp) $(wildcard DataItems/*.cpp) -lm -lpthread -latomic -O3



#aioenetd2:	Makefile $(INCS) $(SRCS)
#	$(GCC) $(FLAGS) -o aioenetd $(SRCS) -lm -lpthread -latomic -O3

#clean:
#	rm -f test aioenetd


all: aioenetd

#GCC doesn't include libstdc++, so just override it here
GCC := aarch64-linux-g++

test:	test.cpp Makefile eNET-types.h TMessage.cpp TMessage.h TError.h TError.cpp apcilib.cpp logging.h adc.cpp adc.h
	$(GCC) -g -Wfatal-errors -std=gnu++2a -o test test.cpp TError.cpp logging.cpp TMessage.cpp apcilib.cpp adc.cpp -lm -lpthread -latomic -O3



aioenetd:	Makefile $(wildcard *.h) $(wildcard *.cpp) $(wildcard DataItems/*.cpp) $(wildcard DataItems/*.h)
	$(GCC) -g -Wfatal-errors -std=gnu++2a -o aioenetd $(wildcard *.cpp) $(wildcard DataItems/*.cpp) -lm -lpthread -latomic -O3

clean:
	rm -f test aioenetd
