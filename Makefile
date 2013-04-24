CFLAGS=-Wall -pedantic -std=c99 -D_XOPEN_SOURCE

all: socketUDP.o tftp.o

socketUDP.o : socketUDP.c socketUDP.h socketUDP_utils.h

socketUDP_utils.o: socketUDP_utils.c socketUDP_utils.h

tftp.o: tftp.c tftp.h socketUDP.h 

clean:
	rm *.o