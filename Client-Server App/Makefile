CC=g++
CFLAGS= -Werror
SRV=server
SBS=subscriber

build: $(SRV) $(SBS)

$(SRV): server.o
	$(CC) $(CFLAGS) $^ -o $(SRV)

$(SBS): subscriber.o
	$(CC) $(CFLAGS) $^ -o $(SBS)

server.o: server.cpp helpers.h
	$(CC) $(CFLAGS) -c server.cpp

subscriber.o: subscriber.cpp helpers.h
	$(CC) $(CFLAGS) -c subscriber.cpp

clean:
	rm -rf subscriber.o server.o $(SRV) $(SBS)
