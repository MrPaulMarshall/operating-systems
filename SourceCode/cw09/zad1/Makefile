CC = gcc -Wall

all:
	make compile
	make run K=2 N=15  || true
	make clean

allArgs:
	make compile
	make run K=${K} N=${N} || true
	make clean

clean:
	rm -f main || true
	rm -f *.o || true

compile:
	$(CC) main.c -o main -lpthread

run:
	./main ${K} ${N}
