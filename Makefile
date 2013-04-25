CFLAGS=-Wall -pedantic -std=c99 -D_XOPEN_SOURCE
LIBS=-lpthread

all: server

server: server.o socketUDP.o socketUDP_utils.o tftp.o

server.o: server.c socketUDP.h tftp.h commons.h

socketUDP.o : socketUDP.c socketUDP.h socketUDP_utils.h

socketUDP_utils.o: socketUDP_utils.c socketUDP_utils.h

tftp.o: tftp.c tftp.h socketUDP.h 

clean:
	rm *.o