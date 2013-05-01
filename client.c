#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "socketUDP.h"
#include "commons.h"
#include "tftp.h"

#define TIMEOUT 10
#define CLIENT_PATH "downloads"

int lostNb = 0;

void writingRoutineOctet(const char *filename, SocketUDP *socket, const char *servAddr);
void readingRoutineOctet(const char *filename, SocketUDP *socket, const char *servAddr);
void writingRoutineNetascii(const char *filename, SocketUDP *socket, const char *servAddr);
void readingRoutineNetascii(const char *filename, SocketUDP *socket, const char *servAddr);

int main(int argc, char** argv) {
  if (argc != 5) {
    fprintf(stderr, "Mauvais nombre d'arguments.\n");
    fprintf(stderr, "Usage: %s serverAddress request mode filename\n", argv[0]);
    fprintf(stderr, "\tserverAddr:\tl'adresse du serveur, addresse IP ou nom de domaine\n");
    fprintf(stderr, "\trequest:\tLe type de requete. read->lecture du fichier depuis le serveur ; write->ecriture sur le serveur\n");
    fprintf(stderr, "\tmode:\t\tLe mode d'échange, octet ou netascii\n");
    fprintf(stderr, "\tfilename:\tLe nom du fichier à échanger\n");
    exit(EXIT_FAILURE);
  }

  SocketUDP *socket = creerSocketUDP();
  if (socket == NULL) {
    fprintf(stderr, "Echec de la création de la Socket UDP attachée.\n");
    exit(EXIT_FAILURE);
  }

  char *requestedFile = argv[4];
  char *mode = argv[3];
  if (strcmp(argv[2], "read") == 0) {

    if (strcmp(mode, "octet") == 0) {
      readingRoutineOctet(requestedFile, socket, argv[1]);
    } else if (strcmp(mode, "netascii") == 0) {
      readingRoutineNetascii(requestedFile, socket, argv[1]);
    } else {
      fprintf(stderr, "Mode inconnu\n");
    }

  } else if (strcmp(argv[2], "write") == 0) {
    if (strcmp(mode, "octet") == 0) {
      writingRoutineOctet(requestedFile, socket, argv[1]);
    } else if (strcmp(mode, "netascii") == 0) {
      writingRoutineNetascii(requestedFile, socket, argv[1]);
    } else {
      fprintf(stderr, "Mode inconnu\n");
    }
  } else {
    fprintf(stderr, "Requete inconnue\n");
  }

  exit(EXIT_SUCCESS);
}

void readingRoutineOctet(const char* filename, SocketUDP *socket, const char *servAddr) {
  int transmissionEnd = 0;
  char serviceAddr[SHORT_BUFFER_LEN];
  int servicePort;
  int blockNb = 1;
  int request = 1;
  int endTime;
  tftp_rwrq toSendPacket;
  tftp_ack ack;
  tftp_packet packet;

  createRWRQ(&toSendPacket, RRQ,
          strlen(filename), filename,
          strlen("octet"), "octet");
  
  //On prépare le fichier côté client
  char filenameBuff[SHORT_BUFFER_LEN];
  snprintf(filenameBuff, SHORT_BUFFER_LEN, "%s/%s", CLIENT_PATH, filename);
  int fd = open(filenameBuff, O_CREAT | O_TRUNC | O_WRONLY);
  if ((fd == -1) || (chmod(filenameBuff, S_IRUSR | S_IWUSR) == -1)) {
    fprintf(stderr, "Erreur de création du fichier, abandon\n");
    exit(EXIT_FAILURE);
  }
  while (!transmissionEnd) {
    int readNb;
    if (request) {
      request = 0;
      if ((readNb = sendAndWait((tftp_packet *) & toSendPacket, &packet, TIMEOUT,
              socket, servAddr, TFTP_SERVER_PORT, &endTime,
              serviceAddr, &servicePort)) == -1) {
        fprintf(stderr, "Le serveur de répond pas ou l'échange à rencontré un problème, abandon\n");
        close(fd);
        unlink(filenameBuff);
        exit(EXIT_FAILURE);
      }
    } else {
      if ((readNb = sendLoop((tftp_packet *) &ack, &packet, TIMEOUT,
                              socket, serviceAddr, servicePort)) == -1) {
        fprintf(stderr, "Le serveur ne répond plus, abandon\n");
        close(fd);
        unlink(filenameBuff);
        exit(EXIT_FAILURE);
      }
    }
    
    if (packet.opCode == ERROR) {
      tftp_error *err = (tftp_error *) & packet;
      fprintf(stderr, "Paquet d'erreur recu : %d - %s\n", err->errorCode, err->errMsg);
      close(fd);
      unlink(filenameBuff);
      exit(EXIT_FAILURE);
    } else if (packet.opCode == DATA) {
      tftp_data *data = (tftp_data *) & packet;
      if (data->blockNb == blockNb) {
        //Condition de fin de transmission
        transmissionEnd = (data->datalen < DATA_LEN);
        //Ecriture du fichier
        int writed;
        if ((writed = write(fd, data->data, data->datalen)) == -1) {
          fprintf(stderr, "Erreur d'écriture du fichier, abandon\n");
          close(fd);
          unlink(filenameBuff);
          exit(EXIT_FAILURE);
        }
        if (writed != data->datalen) {
          fprintf(stderr, "Erreur d'écriture dans le fichier : taille incorrecte, abandon\n");
          close(fd);
          unlink(filenameBuff);
          exit(EXIT_FAILURE);
        }
        createACK(&ack, blockNb);
        blockNb++;
      }
    }
  }
  writePacket((tftp_packet *) &ack, socket, serviceAddr, servicePort);
  close(fd);
}

void writingRoutineOctet(const char* filename, SocketUDP *socket, const char *servAddr) {
  /*  createRWRQ(&requestPacket, WRQ,
                  strlen(requestedFile), requestedFile,
                  strlen(mode), mode);
    
      //Requete de connexion
      writePacket((tftp_packet *) &requestPacket, socket, serverAddr, TFTP_SERVER_PORT);*/
}

void readingRoutineNetascii(const char* filename, SocketUDP *socket, const char *servAddr) {

}

void writingRoutineNetascii(const char* filename, SocketUDP *socket, const char *servAddr) {
  /*createRWRQ(&requestPacket, WRQ,
                strlen(requestedFile), requestedFile,
                strlen(mode), mode);
    
    //Requete de connexion
    writePacket((tftp_packet *) &requestPacket, socket, serverAddr, TFTP_SERVER_PORT);*/
}
