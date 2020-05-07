CC = gcc -Wall

clean:
	rm -f *.o || true
	rm -f *.a || true
	rm -f client || true
	rm -f server || true

compile:
	$(CC) -c library.c
	ar rcs liblibrary.a library.o
	$(CC) client.c -L. -llibrary -o client
	$(CC) server.c -L. -llibrary -o server

runServer:
	./server

runClient:
	./client