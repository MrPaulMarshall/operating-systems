CC = gcc -Wall

all:
	make compile
	make run
	make clean

clean:
	rm -f *.o || true
	rm -f *.a || true
	rm -f manager || true
	rm -f worker_1 || true
	rm -f worker_2 || true
	rm -f worker_3 || true

compile:
	$(CC) -c library.c
	ar rcs liblibrary.a library.o
	$(CC) manager.c -L. -llibrary -o manager -lpthread -lrt
	$(CC) worker_1.c -L. -llibrary -o worker_1 -lpthread -lrt
	$(CC) worker_2.c -L. -llibrary -o worker_2 -lpthread -lrt
	$(CC) worker_3.c -L. -llibrary -o worker_3 -lpthread -lrt

run:
	./manager
