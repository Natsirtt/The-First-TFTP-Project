CFLAGS = -Wall -pedantic -std=c99 CFLAGS = -Wall -pedantic -std=c99 
LIBS = -lpthread

socketUDP.o : socketUDP.c socketUDP.h socketUDP_utils.h

socketUDP_utils.o: socketUDP_utils.c socketUDP_utils.h
