CC = gcc -Wall

clean:
	rm *.o
	rm *.a
	rm main

static:
	$(CC) -c mylib.c
	ar rcs libmylib.a mylib.o
	$(CC) main.c -g -L. -lmylib -o main

run:
	make static
	./main generate ${source} ${lines} ${length}
	./main copy ${source} ${copySys} ${lines} ${length} sys
	./main copy ${source} ${copyLib} ${lines} ${length} lib
	./main sort ${copySys} ${lines} ${length} sys >> wyniki.txt
	./main sort ${copyLib} ${lines} ${length} lib >> wyniki.txt
	echo '-----------------------------------' >> wyniki.txt
	echo '' >> wyniki.txt
	rm ${source}
	rm ${copySys}
	rm ${copyLib}

runTest:
	make static
	./main generate ${source} ${lines} ${length}
	./main copy ${source} ${copySys} ${lines} ${length} sys
	./main copy ${source} ${copyLib} ${lines} ${length} lib
	./main sort ${copySys} ${lines} ${length} sys
	./main sort ${copyLib} ${lines} ${length} lib
	# rm ${source}
	# rm ${copySys}
	# rm ${copyLib}

runAll:
	make run source=${source} copySys=${copySys} copyLib=${copyLib} lines=${lines1} length=1
	make run source=${source} copySys=${copySys} copyLib=${copyLib} lines=${lines4} length=4
	make run source=${source} copySys=${copySys} copyLib=${copyLib} lines=${lines512} length=512
	make run source=${source} copySys=${copySys} copyLib=${copyLib} lines=${lines1024} length=1024
	make run source=${source} copySys=${copySys} copyLib=${copyLib} lines=${lines4096} length=4096
	make run source=${source} copySys=${copySys} copyLib=${copyLib} lines=${lines8192} length=8192


