#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#include "socketUDP.h"
#include "commons.h"
#include "tftp.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "Mauvais nombre d'arguments. Veuillez passer l'addresse du serveur en argument\n");
    exit(EXIT_FAILURE);
  }
  char *serverAddr = argv[1];
  SocketUDP *bindedSocket = creerSocketUDPattachee(INADDR_ANY, TFTP_CLIENT_PORT);
  if (bindedSocket == NULL) {
    fprintf(stderr, "Echec de la création de la Socket UDP attachée.\n");
    exit(EXIT_FAILURE);
  }
  tftp_rwrq requestPacket;
  char *requestedFile = "test";
  char *mode = "octet";
  createRWRQ(&requestPacket, RRQ, strlen(requestedFile), requestedFile,
                                  strlen(mode), mode);
  writePacket((tftp_packet *) &requestPacket, bindedSocket, serverAddr, TFTP_SERVER_PORT);
  
//  exit (EXIT_SUCCESS);
}
