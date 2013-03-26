#ifndef SOCKETUDP_UTILS_H
#define	SOCKETUDP_UTILS_H

#include <netdb.h>

#include "socketUDP.h"


/**
 * Alloue une socket en mode non connecté et retourne le pointeur
 * vers cette socket.
 */
SocketUDP *allocSocketUDP();

/**
 * Libère les ressources d'une socketUDP.
 */
void freeSocketUDP(SocketUDP *socket);

/**
 * Remplit l'id passé en paramètre.
 * Retourne -1 si cela à échoué.
 */
int fillIdUDP(SocketUDP *socket);

/**
 * Initialise la structure sockaddr_in en fonction de l'adresse et du port passés
 * en paramètre.
 * Retourne -1 en cas d'erreur.
 */
int initSockAddrUDP(const char *adresse, int port, struct sockaddr_in *in);

#endif	/* SOCKETUDP_UTILS_H */

