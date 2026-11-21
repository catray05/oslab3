CC= g++ -g

all: producer consumer
	$(CC) -o all
producer: producer.cpp producer.o
	$(CC) -o producer producer.o
consumer: consumer.cpp consumer.o
	$(CC) -o consumer consumer.o
producer.o: producer.cpp 
	$(CC) -c producer.cpp $(COMMNAME) $(PRICEMEAN) $(PRICESTDDEV) $(SLEEPINT) $(SIZE)
consumer.o: consumer.cpp
	$(CC) -c consumer.cpp $(SIZE)