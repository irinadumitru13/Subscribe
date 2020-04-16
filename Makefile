# Protocoale de comunicatii:
# Tema 2
# Makefile

CFLAGS = -Wall -g -std=c++11

#Portul serverului
PORT = 10000

all: server subscriber

server: server.cpp
	g++ ${CFLAGS} server.cpp -o server

subscriber: tcp_client.cpp
	g++ ${CFLAGS} tcp_client.cpp -o subscriber

.PHONY: clean run_server

run_server:
	./server ${PORT}

clean:
	rm -f server subscriber