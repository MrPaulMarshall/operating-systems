# Mam nadzieje ze dziala
CC = gcc -Wall

static:
	make clean
	$(CC) -c mylib.c
	ar rcs libmylib.a mylib.o
	$(CC) Main.c -L. -lmylib -o main -O${opt}

shared:
	make clean
	$(CC) -c -fPIC mylib.c
	$(CC) -shared -fPIC -o libmylib.so mylib.o
	$(CC) Main.c -o main -L. -lmylib -Wl,-rpath=`pwd` -O${opt}

dynamic:
	make clean
	$(CC) -fPIC -c mylib.c
	$(CC) -shared -fPIC -o libmylib.so mylib.o
	$(CC) MainDynamic.c -g -o main -L. -ldl -O${opt}

clean:
	rm -f *.o
	rm -f *.a
	rm -f *.so
	rm -f tempFile.txt
	rm -f main

run1:
	./main create_table 3 compare_pairs text1.txt:text2.txt A1.txt:B1.txt A2.txt:B2.txt remove_operation 2 3 remove_block 1 remove_operation 0 0
	
run2:
	./main create_table 4 compare_pairs text1.txt:text2.txt A1.txt:B1.txt A2.txt:B2.txt A3.txt:B3.txt remove_operation 3 2 remove_block 1 remove_operation 0 1 remove_block 0 remove_block 1 remove_operation 0 0 remove_block 0
	
run3:
	./main create_table 1 compare_pairs A3.txt:B3.txt

run4:
	./main create_table 5 compare_pairs text1.txt:text2.txt A1.txt:B1.txt A2.txt:B2.txt A3.txt:B3.txt A4.txt:B4.txt remove_operation 4 3 remove_block 1 remove_operation 3 2 remove_block 1 remove_operation 0 1 remove_block 0 remove_block 1 remove_operation 0 0 remove_block 0

runAll:
	make run1
	make run2
	make run3
	make run4

staticTest:
	make static opt=${arg}
	make runAll

sharedTest:
	make shared opt=${arg}
	make runAll

dynamicTest:
	make dynamic opt=${arg}
	make runAll

	