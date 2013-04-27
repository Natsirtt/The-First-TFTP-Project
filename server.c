#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <netinet/in.h>
#include <string.h>

#include "socketUDP.h"
#include "commons.h"
#include "tftp.h"

#define SHORT_BUFFER_LEN 256

int lostNumber = 0;

typedef struct {
  id_UDP remote;
  char filename[SHORT_BUFFER_LEN];
  char mode[SHORT_BUFFER_LEN];
} threadArgument_t;

void sendError(SocketUDP *socket, const char *address, int port, int errCode, const char *msg) {
  tftp_error errPac;
  if (createERR(&errPac, errCode, strlen(msg), msg) == -1) {
    fprintf(stderr, "La création d'un paquet d'erreur à échoué\n");
  } else {
    printf("Ecriture d'une erreur sur la socket\n");
    writePacket((tftp_packet *) &errPac, socket, address, port);
    printf("\tdone.\n");
  }
}

void *thread_readingRequestRoutine(void *threadArg);
void *thread_writingRequestRoutine(void *threadArg);

int main(int argc, char** argv) {
  //La socket du serveur
  SocketUDP *bindedSocket = creerSocketUDPattachee(INADDR_ANY, TFTP_SERVER_PORT);
  if (bindedSocket == NULL) {
    fprintf(stderr, "Echec de la création de la Socket UDP attachée.\n");
    exit(EXIT_FAILURE);
  }
  printf("Socket UDP attachée sur %s(%s):%d\n",
          getLocalIP(bindedSocket), getLocalName(bindedSocket),
          getLocalPort(bindedSocket));

  while (1) {
    tftp_packet packet;
    char address[SHORT_BUFF_LEN];
    int port;
    if (readPacket(&packet, bindedSocket, address, &port) == -1) {
      printf("Echec de lecture d'un paquet");
      lostNumber++;
    } else {
      printf("Paquet de requete recu, tests sur le paquet en cours\n");
      if ((packet.opCode != RRQ) && (packet.opCode != WRQ)) {
        printf("Requete incorrecte recue, envoi d'une erreur\n");
        sendError(bindedSocket, address, port, ILLOP_ERR, "Requete incorrecte");
      } else {
        tftp_rwrq *reqPacket = (tftp_rwrq *) & packet;
        printf("mode: <%s>\n", reqPacket->mode);
        if ((strcmp(reqPacket->mode, "octet") != 0) && (strcmp(reqPacket->mode, "netascii") != 0)) {
          printf("Mode recu inconnu, envoi d'une erreur\n");
          sendError(bindedSocket, address, port, ILLOP_ERR, "Mode de lecture inconnu");
        } else {
          //threader et traiter le paquet
          printf("Paquet correct, creation d'un thread\n");
          pthread_t thread;
          id_UDP remoteID;
          remoteID.ip = address;
          remoteID.port = port;
          threadArgument_t arg;
          arg.remote = remoteID;
          strncpy(arg.filename, reqPacket->data, SHORT_BUFFER_LEN);
          strncpy(arg.mode, reqPacket->mode, SHORT_BUFFER_LEN);
          if (reqPacket->opCode == RRQ) {
            pthread_create(&thread, NULL, thread_readingRequestRoutine, &arg);
          } else {
            pthread_create(&thread, NULL, thread_writingRequestRoutine, &arg);
          }
        }
      }
    }
  }
}

void *thread_readingRequestRoutine(void* threadArg) {
  threadArgument_t *arg = (threadArgument_t *) threadArg;
  printf("Paquet de requête de lecture reçu depuis %s:%d\n", arg->remote.ip, arg->remote.port);
  printf("Nom du fichier : %s - mode : %s\n", arg->filename, arg->mode);

  return NULL;
}

void *thread_writingRequestRoutine(void* threadArg) {
  threadArgument_t *arg = (threadArgument_t *) threadArg;


  return NULL;
}
