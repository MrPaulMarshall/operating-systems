CC = gcc -Wall

clean:
	rm *.o
	rm *.a
	rm myFind

staticDIR:
	# make clean
	$(CC) -c mylib.c
	ar rcs libmylib.a mylib.o
	$(CC) mainDIR.c -g -L. -lmylib -o myFind

staticNFTW:
	# make clean
	$(CC) -c mylib.c
	ar rcs libmylib.a mylib.o
	$(CC) mainNFTW.c -g -L. -lmylib -o myFind
