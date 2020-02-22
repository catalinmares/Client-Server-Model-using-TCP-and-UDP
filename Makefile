CC = gcc
CFLAGS = -Wall -g

build: subscriber server

subscriber: subscriber.o utils.o
	$(CC) $(CFLAGS) subscriber.o utils.o -lm -o $@

server: server.o utils.o
	$(CC) $(CFLAGS) server.o utils.o -lm -o $@

.c.o:
	gcc -Wall -g -c $?

clean:
	rm -f server subscriber
