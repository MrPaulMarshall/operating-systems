# Moj pierwszy plik Makefile
# Mam nadzieje ze dziala
CC = gcc -Wall

static:
	make clean
	$(CC) -c mylib.c
	ar rcs libmylib.a mylib.o

shared:
	make clean
	$(CC) -fPIC -c mylib.c
	$(CC) -shared -fPIC -o mylib.so mylib.o

clean:
	rm -f *.o
	rm -f *.a
	rm -f *.so
