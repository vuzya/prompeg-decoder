all: client.cpp
	g++ -g -pthread -std=c++11 client.cpp -o client
clean:
	rm -f client
