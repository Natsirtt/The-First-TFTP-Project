#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "socketUDP_utils.h"

#define SHORT_BUFF_LENGTH 256

SocketUDP *allocSocketUDP() {
  SocketUDP *socket;
  socket = (SocketUDP *) malloc(sizeof(SocketUDP));
  if (socket != NULL) {
    socket->socket = -1;
    socket->local.name = NULL;
    socket->local.ip = NULL;
    socket->local.port = -1;
  }
  return socket;
}

void _freeIdUDP(id_UDP *i) {
  if (i->name != NULL) {
    free(i->name);
  }
  
  if (i->ip != NULL) {
    free(i->ip);
  }
}

void freeSocketUDP(SocketUDP *socket) {
  if (socket != NULL) {
    _freeIdUDP(&(socket->local));
    free(socket);
  }
}

int fillIdUDP(SocketUDP *socket) {
  struct sockaddr_in in;
  socklen_t length = sizeof(struct sockaddr_in);
  
  if (getsockname(socket->socket, (struct sockaddr *) &in, &length) == -1) {
    return -1;
  }
  
  char host[SHORT_BUFF_LENGTH];
  char serv[SHORT_BUFF_LENGTH];
  if (getnameinfo((struct sockaddr *) &in, sizeof(struct sockaddr_in),
                  host, SHORT_BUFF_LENGTH, serv, SHORT_BUFF_LENGTH,
          NI_NUMERICSERV | NI_DGRAM) != 0) {
    return -1;
  }
  
  //Nom de domaine de la machine
  size_t hostlen = strlen(host);
  socket->local.name = (char *) malloc(hostlen + 1);
  strncpy(socket->local.name, host, hostlen);
  socket->local.name[hostlen] = 0;
  
  //Port
  socket->local.port = atoi(serv);
  
  if (getnameinfo((struct sockaddr *) &in, sizeof(struct sockaddr_in),
                  host, SHORT_BUFF_LENGTH, NULL, 0,
                  NI_NUMERICHOST | NI_DGRAM) != 0) {
    free(socket->local.name);
    socket->local.port = -1;
    return -1;
  }
  size_t ipLen = strlen(host);
  socket->local.ip = (char *) malloc(ipLen + 1);
  strncpy(socket->local.ip, host, ipLen);
  socket->local.ip[ipLen] = 0;
  
  return 0;
}

int initSockAddrUDP(const char *adresse, int port, struct sockaddr_in *in) {
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
//  memset(in, 0, sizeof(struct sockaddr_in));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_ADDRCONFIG | AI_PASSIVE;
  char p[SHORT_BUFF_LENGTH];
  snprintf(p, SHORT_BUFF_LENGTH, "%d", port);
  
  struct addrinfo *res;
  if (getaddrinfo(adresse, p, &hints, &res) != 0) {
    return -1;
  }
  
  in = memcpy(in, res->ai_addr, sizeof(struct sockaddr_in));
  freeaddrinfo(res);
  return 0;
}