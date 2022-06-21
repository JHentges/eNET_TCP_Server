all: test

test:	test.cpp parser.h diag.h parser.cpp Messages.cpp utility.cpp consts.cpp 
	g++ -std=gnu++17 -o test test.cpp parser.cpp Messages.cpp utility.cpp consts.cpp -lm -lpthread -O3

eNET-TCP-Server:	eNET-TCP-Server.cpp parser.cpp Messages.cpp utility.cpp consts.cpp test.cpp
	gcc -std=gnu++17 -o eNET-TCP-Server eNET-TCP-Server.cpp parser.cpp Messages.cpp utility.cpp consts.cpp test.cpp -lm -lpthread -O3

clean:
	rm -f test