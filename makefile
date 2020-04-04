all: server client

server: common.c  common.h  list.c  list.h  server.c
	gcc server.c common.c list.c -std=c99 -o server -lrt -lpthread

client: client.c  common.c  common.h  list.c  list.h
	gcc client.c common.c list.c -std=c99 -o client -lrt -lpthread

clean:
	rm -rf client server