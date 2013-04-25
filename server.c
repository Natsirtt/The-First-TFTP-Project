#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <netinet/in.h>
#include <string.h>

#include "socketUDP.h"
#include "commons.h"
#include "tftp.h"

int main(int argc, char** argv) {
  //La socket du serveur
  SocketUDP *bindedSocket = creerSocketUDPattachee(INADDR_ANY, TFTP_PORT);
  if (bindedSocket == NULL) {
    fprintf(stderr, "Echec de la création de la Socket UDP d'écoute.\n");
    exit(EXIT_FAILURE);
  }
  printf("Socket UDP en écoute sur %s(%s):%d\n",
              getLocalIP(bindedSocket), getLocalName(bindedSocket),
              getLocalPort(bindedSocket));
  
  while (1) {
    tftp_packet packet;
    char address[SHORT_BUFF_LEN];
    int port;
    int res = readPacket(&packet, bindedSocket, address, &port);
    if ((packet.opCode != RRQ) || (packet.opCode != WRQ)) {
      tftp_error errPac;
      char *msg = "Requête incorrecte";
      createERR(&errPac, 4, strlen(msg), msg);
      writePacket((tftp_packet *) &errPac, bindedSocket, address, port);
    }
    //threader et traiter le paquet
  }
  return (EXIT_SUCCESS);
}

