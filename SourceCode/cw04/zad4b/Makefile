CC = gcc -Wall

clean:
	rm sender
	rm catcher

compile:
	$(CC) catcher.c -o catcher
	$(CC) sender.c -o sender

test:
	./catcher ${mode} & pid=$$!; \
	#echo $$pid; \
	echo; \
	sleep 1; \
	./sender $$pid ${n} ${mode}
