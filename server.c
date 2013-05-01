#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "socketUDP.h"
#include "commons.h"
#include "tftp.h"

#define SERVER_PATH "files/"

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
    writePacket((tftp_packet *) & errPac, socket, address, port);
    printf("done.\n");
  }
}

void *thread_readingRequestRoutineOctet(void *threadArg);
void *thread_writingRequestRoutineOctet(void *threadArg);
void *thread_readingRequestRoutineNetascii(void* threadArg);
void *thread_writingRequestRoutineNetascii(void* threadArg);

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
      printf("Echec de lecture d'un paquet\n");
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
          strncpy(arg.filename, SERVER_PATH, strlen(SERVER_PATH));
          strncat(arg.filename, reqPacket->data, SHORT_BUFFER_LEN);
          strncpy(arg.mode, reqPacket->mode, SHORT_BUFFER_LEN);
          //Options de thread pour faire un thread détaché
          pthread_attr_t thread_attr;
          pthread_attr_init(&thread_attr);
          pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
          if (strcmp(reqPacket->mode, "octet") == 0) {
            if (reqPacket->opCode == RRQ) {
              pthread_create(&thread, &thread_attr, thread_readingRequestRoutineOctet, &arg);
            } else {
              pthread_create(&thread, &thread_attr, thread_writingRequestRoutineOctet, &arg);
            }
          } else {
            if (reqPacket->opCode == RRQ) {
              pthread_create(&thread, &thread_attr, thread_readingRequestRoutineNetascii, &arg);
            } else {
              pthread_create(&thread, &thread_attr, thread_readingRequestRoutineNetascii, &arg);
            }
          }
        }
      }
    }
  }
}

void *thread_readingRequestRoutineOctet(void* threadArg) {
  threadArgument_t *arg = (threadArgument_t *) threadArg;
  //Création d'une socket spécifique à cette connexion
  SocketUDP *serviceSocket = creerSocketUDP();
  int end = 0;
  
  int fd = open(arg->filename, O_RDONLY);
  if (fd == -1) {
    if (errno == ENOENT) {
      char errMsg[1024];
      fprintf(stderr, "Le fichier %s est introuvable\n", arg->filename);
      snprintf(errMsg, 1024, "Le fichier %s n'a pas été trouvé sur le serveur",
                              arg->filename);
      sendError(serviceSocket, arg->remote.ip, arg->remote.port, FNF_ERR, errMsg);
    } else {
      sendError(serviceSocket, arg->remote.ip, arg->remote.port, UKNW_ERR,
                              "Erreur lors de l'ouverture du fichier");
    }
    end = 1;
  }

  //Lecture du fichier et 
  int dataBlocNb = 1;
  while (end != 1) {
    tftp_data data;
    char buff[DATA_LEN];
    
    int readNb;
    if ((readNb = read(fd, buff, DATA_LEN)) == -1) {
      sendError(serviceSocket, arg->remote.ip, arg->remote.port, UKNW_ERR,
                "Erreur interne du serveur (lecture échouée)");
      end = 1;
      continue;
    }
    
    end = (readNb < DATA_LEN);
    
    if (createDATA(&data, dataBlocNb, readNb, buff) == -1) {
      sendError(serviceSocket, arg->remote.ip, arg->remote.port, UKNW_ERR,
                "Erreur interne du serveur (création du paquet échouée)");
      end = 1;
      continue;
    }
    
    printf("buff = <%s> et data = <%s>\n", buff, data.data);
    tftp_packet packet;
    int res = sendLoop((tftp_packet *) &data, &packet, TIMEOUT,
              serviceSocket, arg->remote.ip, arg->remote.port);
    if (res <= 0) {
      sendError(serviceSocket, arg->remote.ip, arg->remote.port,
                UKNW_ERR, "Tout les essais d'envoi du paquet sont un échec");
    }
    
    //Si on est ici, on a un ACK correspondant au bon blockNb (controlé par sendLoop)
    //ou bien une erreur.
    dataBlocNb++;
    if (packet.opCode == ERROR) {
      fprintf(stderr, "Le client termine la connexion par une erreur\n");
      end = 1;
      continue;
    }    
  }
  close(fd);
  return NULL;
}

void *thread_writingRequestRoutineOctet(void* threadArg) {
  //  threadArgument_t *arg = (threadArgument_t *) threadArg;
  //Création d'une socket spécifique à cette connexion
  //  SocketUDP *serviceSocket = creerSocketUDP();

  return NULL;
}

void *thread_readingRequestRoutineNetascii(void* threadArg) {
  //  threadArgument_t *arg = (threadArgument_t *) threadArg;
  //Création d'une socket spécifique à cette connexion
  //  SocketUDP *serviceSocket = creerSocketUDP();


  return NULL;
}

void *thread_writingRequestRoutineNetascii(void* threadArg) {
  //  threadArgument_t *arg = (threadArgument_t *) threadArg;
  //Création d'une socket spécifique à cette connexion
  //  SocketUDP *serviceSocket = creerSocketUDP();

  return NULL;
}
