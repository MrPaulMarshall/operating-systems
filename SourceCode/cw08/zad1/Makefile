CC = gcc -Wall
InF = barbara.ascii.pgm
out = ./results/histogram

all:
	make compile
	mkdir results								|| true
	make run c_thr=1 div_opt=sign        input=$(InF) output=$(out)_s_1.txt || true
	make run c_thr=2 div_opt=sign        input=$(InF) output=$(out)_s_2.txt || true
	make run c_thr=4 div_opt=sign        input=$(InF) output=$(out)_s_4.txt || true
	make run c_thr=8 div_opt=sign        input=$(InF) output=$(out)_s_8.txt || true
	make run c_thr=1 div_opt=block       input=$(InF) output=$(out)_b_1.txt || true
	make run c_thr=2 div_opt=block       input=$(InF) output=$(out)_b_2.txt || true
	make run c_thr=4 div_opt=block       input=$(InF) output=$(out)_b_4.txt || true
	make run c_thr=8 div_opt=block       input=$(InF) output=$(out)_b_8.txt || true
	make run c_thr=1 div_opt=interleaved input=$(InF) output=$(out)_i_1.txt || true
	make run c_thr=2 div_opt=interleaved input=$(InF) output=$(out)_i_2.txt || true
	make run c_thr=4 div_opt=interleaved input=$(InF) output=$(out)_i_4.txt || true
	make run c_thr=8 div_opt=interleaved input=$(InF) output=$(out)_i_8.txt || true
	make clean

all_args:
	make compile
	make run c_thr=${c_thr} div_opt=${div_opt} input=${input} output=${output} || true
	make clean

clean:
	rm -f main || true
	rm -f *.o || true

rm_outputs:
	rm *.txt || true

compile:
	$(CC) main.c -o main -lpthread

run:
	./main ${c_thr} ${div_opt} ${input} ${output} 
