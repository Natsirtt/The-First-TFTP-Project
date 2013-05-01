#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>

#include "socketUDP.h"
#include "socketUDP_utils.h"

#define SHORT_BUFFER_LEN 256

SocketUDP *creerSocketUDP() {
  SocketUDP *sock = allocSocketUDP();
  sock->socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock->socket == -1) {
    freeSocketUDP(sock);
    return NULL;
  }
  return sock;
}

SocketUDP *creerSocketUDPattachee(const char *address, int port) {
  SocketUDP *sock = creerSocketUDP();

  struct sockaddr_in in;
  if (initSockAddrUDP(address, port, &in) == -1) {
    fprintf(stderr, "socketUDP: creerSocketUDPattachee -> iniSockAddrUDP a retourné -1\n");
    return NULL;
  }
  if (bind(sock->socket, (struct sockaddr *) &in, sizeof (struct sockaddr_in)) == -1) {
    fprintf(stderr, "socketUDP: creerSocketUDPattachee -> bind a retourné -1\n");
    perror("Message ");
    fprintf(stderr, "\n");
    return NULL;
  }
  fillIdUDP(sock);
  return sock;
}

char *getLocalName(SocketUDP *socket) {
  if (socket == NULL) {
    return NULL;
  }
  if (socket->local.name == NULL) {
    return NULL;
  }
  size_t len = strlen(socket->local.name);
  char *res = (char *) malloc(len + 1);
  strncpy(res, socket->local.name, len);
  res[len] = 0;
  return res;
}

char *getLocalIP(SocketUDP *socket) {
  if (socket == NULL) {
    return NULL;
  }
  if (socket->local.ip == NULL) {
    return NULL;
  }
  size_t len = strlen(socket->local.ip);
  char *res = (char *) malloc(len + 1);
  strncpy(res, socket->local.ip, len);
  res[len] = 0;
  return res;
}

int getLocalPort(SocketUDP *socket) {
  if (socket == NULL) {
    return -1;
  }
  return socket->local.port;
}

int writeToSocketUDP(SocketUDP *socket, const char *address, int port,
        const char *buffer, int length) {
  struct sockaddr_in in;
  initSockAddrUDP(address, port, &in);
  printf("Envoi dans la socket...\n");
  int res = sendto(socket->socket, buffer, length, 0, (struct sockaddr *) &in, sizeof (struct sockaddr_in));
  printf("OK\n");
  if (socket->local.port == -1) {
    fillIdUDP(socket);
  }
  return res;
}

int readFromSocketUDP(SocketUDP *socket, char *buffer, int length,
        char *address, int *port, int timeout, int *endTime) {
  //Timeout avec option sur la socket
  if (timeout > 0) {
    struct timeval tval;
    memset(&tval, 0, sizeof (struct timeval));
    tval.tv_sec = timeout;
    setsockopt(socket->socket, SOL_SOCKET, SO_RCVTIMEO, &tval,
            sizeof (struct timeval));
  }
  //Lecture sur la socket
  struct sockaddr_in in;
  socklen_t len = sizeof (struct sockaddr_in);
  struct timeval startTime;
  if (timeout > 0) {
    gettimeofday(&startTime, NULL);
  }
  ssize_t res = recvfrom(socket->socket, buffer, length, 0, (struct sockaddr *) &in, &len);
  if (timeout > 0) {
      struct timeval endtval;
      gettimeofday(&endtval, NULL);
      *endTime = timeout - (endtval.tv_sec - startTime.tv_sec);
  }
  if (res == -1) {
    return -1;
  }
  //Remplissage des infos
  if (port != NULL) {
    *port = ntohs(in.sin_port);
  }

  if (address != NULL) {
    char buff[SHORT_BUFFER_LEN];
    if (getnameinfo((struct sockaddr *) &in, sizeof (struct sockaddr_in), buff, SHORT_BUFFER_LEN - 1, NULL, 0, NI_DGRAM) != 0) {
      return res;
    }
    strncpy(address, buff, SHORT_BUFFER_LEN - 1);
    address[strlen(buff)] = 0;
  }
  
  return res;
}

int closeSocketUDP(SocketUDP *socket) {
  int res = close(socket->socket);
  freeSocketUDP(socket);
  return res;
}
