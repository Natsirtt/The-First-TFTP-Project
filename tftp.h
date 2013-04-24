#ifndef TFTP_H
#define	TFTP_H

#include <sys/types.h>
#include "socketUDP.h"

/**
 * Les types de paquet pour le protocole tftp
 */
#define RRQ 0
#define WRQ 1
#define DATA 2
#define ACK 3
#define ERROR 4

/**
 * Les types d'erreur pour le protocole tftp.
 */
#define UKNW_ERR 0
#define FNF_ERR 1
#define ACCESS_ERR 2
#define DISK_ERR 3
#define ILLOP_ERR 4
#define TID_ERR 5
#define FEXIST_ERR 6
#define USER_ERR 7

#define RWR_DATA 498
#define RWR_MODE 16
#define DATA_LEN 512
#define PACKET_LEN 516

#define SHORT_BUFF_LEN 256

typedef struct {
  int16_t opCode;
  char data[];
} tftp_packet;

typedef struct {
  int16_t opCode;
  int16_t blockNb;
} tftp_ack;

typedef struct {
  int16_t opCode;
  char data[RWR_DATA];
  char mode[RWR_MODE];
} tftp_rwrq;

typedef struct {
  int16_t opCode;
  int16_t blockNb;
  int datalen;
  char data[DATA_LEN];
} tftp_data;

typedef struct {
  int16_t opCode;
  int16_t errorCode;
  char errMsg[DATA_LEN];
} tftp_error;


/*
 * Les fonctions de manipulaion des paquets.
 */
/**
 * Retourne le type du paquet packet.
 */
int getType(tftp_packet *packet);
/**
 * Rend un packet RRQ ou WRQ correctement initialisé.
 */
int createRWRQ(tftp_rwrq *packet, int code, 
	       int flength, char *file, int mlength, char *mode);
/**
 * Rend un packet DATA correctement initialisé.
 */
int createDATA(tftp_data *packet, int nb, int datalen, char *data);
/**
 * Rend un paquet ACK correctement initialisé.
 */
int createACK(tftp_ack *packet, int nb);
/**
 * Rend un paquet ERR correctement initialisé.
 */
int createERR(tftp_error *packet, int errCode, 
	      int messlen, char *message);


/*
 * Les fonctions de communication.
 */
/**
 * Envoie un paquet et attend d'en recevoir un en retour.
 * Le paquet est envoyé par la socket s.
 * Le type des paquet doit être géré interieurment.
 */
int sendAndWait(tftp_packet *send, tftp_packet *wait, 
		int timeout, SocketUDP *s, 
		const char *address, int port, int *endTime);

/**
 * Envoie un paquet d'erreur par la socket s.
 */
int sendError(tftp_error *error, SocketUDP *s, const char *address, int port);

/**
 * Convertie le paquet packet en chaine à écrire sur
 * le réseaux.
 */
int toNetwork(tftp_packet *packet, char *buffer, int len);

/**
 * Convertie une chaine du réseau en un paquet.
 */
int fromNetwork(tftp_packet *packet, int packetLength, char *buffer, int len);

#endif	/* TFTP_H */
