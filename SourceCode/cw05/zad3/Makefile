CC = gcc -Wall

pipe = myPipe
p = producer_data
c = consumer_data

N = 5

clean:
	rm producer || true
	rm consumer || true
	rm $(c)/output.txt || true
	rm $(pipe) || true

compile:
	$(CC) producer.c -o producer
	$(CC) consumer.c -o consumer
	$(CC) main.c -o main

createPipe:
	rm $(pipe) || true
	mkfifo $(pipe)

runProducer:
	./producer $(pipe) ./$(p)/f1.txt $(N)
	# ./producer $(pipe) ./$(p)/f2.txt $(N)
	# ./producer $(pipe) ./$(p)/f3.txt $(N)
	# ./producer $(pipe) ./$(p)/f4.txt $(N)
	# ./producer $(pipe) ./$(p)/f5.txt $(N)

runConsumer:
	./consumer $(pipe) ./$(c)/output.txt 10
	cat ./$(c)/output.txt

runMain:
	./main
	cat ./$(c)/output.txt
