CC = gcc -Wall

all:
	make compile
	make help
	make clean

compile:
	$(CC) client.c -o client -lpthread || true
	$(CC) server.c -o server -lpthread || true

clean:
	rm -f client || true
	rm -f server || true

# P - port, S - Socket
Server:
	sudo ./server ${P} ${S}

# N - client name, M - mode, A - server address, P - port (inet)
Client:
	sudo ./client ${N} ${M} ${A} ${P}

help:
	./server

