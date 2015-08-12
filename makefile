##simple makefile

client: client.o
	g++ client.o -o ./client -lpthread
client.o: client.cpp
	g++ client.cpp -c 
clean:
	rm -rf *.o
