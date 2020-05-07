CC = gcc -Wall
file = main

clean:
	rm -f $(file) || true

compile:
	make clean
	$(CC) main.c -o $(file)

run:
	make compile
	./$(file) text.txt
