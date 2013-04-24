#ifndef SOCKETUDP_H
#define	SOCKETUDP_H

    typedef struct {
        char *name;
        char *ip;
        int port;
    } id_UDP;

    typedef struct {
        int socket;
        id_UDP local;
    } SocketUDP;

    /**
     * Creates a new UDP socket and return the socket descriptor. 
     */
    SocketUDP *creerSocketUDP();

    /**
     * Creates a new UDP socket, binds it to the local address (name or ip) and
     * the local port passed to the function.
     */
    SocketUDP *creerSocketUDPattachee(const char *address, int port);

    /**
     * Returns the local UDP socket's name. 
     */
    char *getLocalName(SocketUDP *socket);

    /**
     * Returns the local UDP socket's ip address.
     */
    char *getLocalIP(SocketUDP *socket);

    /**
     * Returns the local UDP socket's port. 
     */
    int getLocalPort(SocketUDP *socket);

    /**
     * Writes the buffer on the socket to the address (name or IP). Writes
     * length bytes from the buffer.
     * Returns the size of writed data or -1 if there's an error.
     */
    int writeToSocketUDP(SocketUDP *socket, const char *address, int port, 
            const char *buffer, int length);
    
    /**
     * Reads data on the socket from the remote adress:port host. Puts the data
     * in buff at most length bytes.
     * Returns the size of read data or -1 if there's an error.
     */
    int readFromSocketUDP(SocketUDP *socket, char *buffer, int length,
                          char *address, int *port, int timeout, 
                          int *endTime);
    
    /**
     * Free the socket.
     */
    int closeSocketUDP(SocketUDP *socket);

#endif	/* SOCKETUDP_H */

