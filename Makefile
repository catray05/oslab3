CC= g++ -g

all: producer consumer
producer: producer.o
	$(CC) -pthread -o producer producer.o
consumer: consumer.o
	$(CC) -pthread -o consumer consumer.o
producer.o: producer.cpp 
	$(CC) -pthread -c producer.cpp $(COMMNAME) $(PRICEMEAN) $(PRICESTDDEV) $(SLEEPINT) $(SIZE)
consumer.o: consumer.cpp
	$(CC) -pthread -c consumer.cpp $(SIZE)
clean:
	rm -f *.o producer consumer