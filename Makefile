TRGET: siktacka-server siktacka-client

CC      = g++
CC_FLAGS  = -c -Wall -std=c++14
LFLAGS  = -Wall -std=c++14

debug: CC_FLAGS += -DDEBUG -g

err.o: err.h err.c
	$(CC) $(CC_FLAGS) err.c

server.o: server.h server.cpp
	$(CC) $(CC_FLAGS) server.cpp

data_structures.o: data_structures.h data_structures.cpp
	$(CC) $(CC_FLAGS) data_structures.cpp

siktacka.o: siktacka.h siktacka.cpp err.h
	$(CC) $(CC_FLAGS) siktacka.cpp

siktacka-server.o: err.h siktacka.h data_structures.h siktacka-server.cpp
	$(CC) $(CC_FLAGS) siktacka-server.cpp

siktacka-client.o: err.h siktacka.h data_structures.h siktacka-client.cpp
	$(CC) $(CC_FLAGS) siktacka-client.cpp

siktacka-server: siktacka-server.o err.o siktacka.o data_structures.o server.o
	$(CC) $(LFLAGS) $^ -o $@

siktacka-client: siktacka-client.o err.o siktacka.o data_structures.o 
	$(CC) $(LFLAGS) $^ -o $@

debug: siktacka-server siktacka-client

.PHONY: clean TARGET
clean:
	rm -f siktacka-server siktacka-client *.o *~ *.bak


