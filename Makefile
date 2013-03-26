CFLAGS=-Wall -pedantic -std=c99 -D_XOPEN_SOURCE

socketUDP.o : socketUDP.c socketUDP.h socketUDP_utils.h

socketUDP_utils.o: socketUDP_utils.c socketUDP_utils.h
