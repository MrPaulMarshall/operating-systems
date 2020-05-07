CC = gcc -Wall
file = main
path = commands.txt

clean:
	rm -f $(file) || true

compile:
	make clean
	$(CC) main.c -o $(file)

run:
	make compile
	./$(file) $(path)
