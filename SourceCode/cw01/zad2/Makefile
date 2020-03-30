# Moj pierwszy plik Makefile
# Mam nadzieje ze dziala
CC = gcc -Wall

static:
	make clean
	$(CC) -c mylib.c
	ar rcs libmylib.a mylib.o
	$(CC) Main.c -L. -lmylib -o main

shared:
	make clean
	$(CC) -c -fPIC mylib.c
	$(CC) -shared -fPIC -o libmylib.so mylib.o
	$(CC) Main.c -o main -L. -lmylib -Wl,-rpath=`pwd`

clean:
	rm -f *.o
	rm -f *.a
	rm -f *.so
	rm -f main
	rm -f tempFile.txt
