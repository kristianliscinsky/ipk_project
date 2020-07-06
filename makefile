CC = g++ -static-libstdc++
CFLAGS = -Wall -Wextra -pedantic

all: ipk-server ipk-client
	
ipk-server: ipk-server.c
	$(CC) $(CFLAGS) -o $@ ipk-server.c
	
ipk-client: ipk-client.c
	$(CC) $(CFLAGS) -o $@ ipk-client.c
clean:
	rm -f *.o *.out ipk-client ipk-server
