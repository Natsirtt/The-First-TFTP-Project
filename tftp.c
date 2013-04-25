#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <arpa/inet.h>

#include "tftp.h"
#include "socketUDP.h"

int PACK_WAIT[5] = {DATA, ACK, ACK, DATA, -1};

/*
 * Manipulation des paquets.
 */
int getType(tftp_packet *packet) {
  if (packet == NULL) {
    return -1;
  }
  return packet->opCode;
}

int createRWRQ(tftp_rwrq *packet, int code,
	       int flength, char *file, int mlength, char *mode) {
  if (flength > RWR_DATA - 1 || mlength > RWR_MODE - 1) {
    return -1;
  }
  if (code != RRQ && code != WRQ) {
    return -1;
  }
  if (packet == NULL) {
    return -1;
  }
  int netCode = htonl(code);
  packet->opCode = netCode;
  memset(packet->data, 0, RWR_DATA);
  memset(packet->mode, 0, RWR_MODE);
  strncpy(packet->data, file, flength);
  strncpy(packet->mode, file, mlength);  
  return 0;
}

int createDATA(tftp_data *packet, int nb, int datalen, char *data) {
  if (datalen > DATA_LEN - 1) {
    return -1;
  }
  if (packet == NULL) {
    return -1;
  }
  packet->opCode = htonl(DATA);
  packet->blockNb = htonl(nb);
  packet->datalen = datalen;
  memset(packet->data, 0, datalen);
  strncpy(packet->data, data, datalen);

  return 0;
}

int createACK(tftp_ack *packet, int nb) {
  if (packet == NULL) {
    return -1;
  }
  packet->opCode = htonl(ACK);
  packet->blockNb = htonl(nb);

  return 0;
}

int createERR(tftp_error *packet, int errCode, 
	      int messlen, char *message) {
  if (messlen > DATA_LEN - 1) {
    return -1;
  }
  if (packet == NULL) {
    return -1;
  }
  memset(packet->errMsg, 0, DATA_LEN);
  packet->opCode = htonl(ERROR);
  packet->errorCode = htonl(errCode);
  strncpy(packet->errMsg, message, messlen);

  return 0;
}

/*
 * Communication.
 */ 
int sendAndWait(tftp_packet *send, tftp_packet *wait,
		int timeout, SocketUDP *s, 
		const char *address, int port, int *endTime) {
  
  char netPacket[PACKET_LEN];
  if (toNetwork(send, netPacket, PACKET_LEN) == -1) {
    return -1;
  }
  if (writeToSocketUDP(s, address, port, netPacket, PACKET_LEN) == -1) {
    return -1;
  }
  char buff[PACKET_LEN];
  char addr[SHORT_BUFF_LEN];
  int senderPort;
  int time;
  tftp_packet tmp;
  int packetLen;
  if ((packetLen = readFromSocketUDP(s, buff, PACKET_LEN, 
			addr, &senderPort, timeout, &time)) == -1) {
    memcpy(endTime, &time, sizeof(int));
    return -1;
  }
  memcpy(endTime, &time, sizeof(int));
  if (time == 0) {
    return -1;
  }
  fromNetwork(&tmp, packetLen, buff, PACKET_LEN);
  if (tmp.opCode != PACK_WAIT[send->opCode] && tmp.opCode != ERROR) {
    return -1;
  }
  if (send->opCode != RRQ && send->opCode != WRQ) {
    if ((strcmp(addr, address) != 0) || port != senderPort) {
      tftp_error errPac;
      char *msg = "Unexpected address and port";
      createERR(&errPac, 5, strlen(msg), msg);
      toNetwork((tftp_packet *) &errPac, buff, PACKET_LEN);
      writeToSocketUDP(s, addr, senderPort, buff, PACKET_LEN);
      return -2;
    }
  }
  memcpy(wait, &tmp, PACKET_LEN);
  return 0;
}

int sendLoop(tftp_packet* toSend, tftp_packet* toReceive,
            int timeout, SocketUDP* s, const char* address,
            int port) {
  int res = -1;
  for (int i = 0; (i < SEND_LOOP_NB) && (res != 0); i++) {
    int endTime;
    res = sendAndWait(toSend, toReceive, timeout, s, address, port, &endTime);
    //Si res == 0, on sortira de la boucle, ce sera terminé.
    //Si res == -1, la boucle recommencera pour le prochain essai
    //Si res == -2, on relance cet essai en diminuant le timeout
    while (res == -2) {
      printf("TFTP: res == -2 / retrying with timeout=%d", endTime);
      int newTimeout = endTime;
      res = sendAndWait(toSend, toReceive, newTimeout, s, address, port, &endTime);
    }
  }
  return res;
}

int toNetwork(tftp_packet *packet, char *buffer, int len) {
  if (packet == NULL || buffer == NULL || len < PACKET_LEN) {
    return -1;
  }
  switch (packet->opCode) {
  case RRQ:
  case WRQ: {
    tftp_rwrq *pac = (tftp_rwrq *) packet;
    memset(buffer, 0, PACKET_LEN);
    memcpy(buffer, &(pac->opCode), 2);
    memcpy(buffer + 2, pac->data, RWR_DATA);
    memcpy(buffer + 2 + RWR_DATA, pac->mode, RWR_MODE);
    break;
  }
  case DATA: {
    tftp_data *pac = (tftp_data *) packet;
    memset(buffer, 0, PACKET_LEN);
    memcpy(buffer, &(pac->opCode), 2);
    memcpy(buffer + 2, &(pac->blockNb), 2);
    memcpy(buffer + 4, pac->data, pac->datalen);
    break;
  }
  case ACK: {
    tftp_ack *pac = (tftp_ack *) packet;
    memset(buffer, 0, PACKET_LEN);
    memcpy(buffer, &(pac->opCode), 2);
    memcpy(buffer + 2, &(pac->blockNb), 2);
    break;
  }
  case ERROR: {
    tftp_error *pac = (tftp_error *) packet;
    memset(buffer, 0, PACKET_LEN);
    memcpy(buffer, &(pac->opCode), 2);
    memcpy(buffer + 2, &(pac->errorCode), 2);
    memcpy(buffer + 4, pac->errMsg, DATA_LEN);
    break;
  }
  default :
    return -1;
    break;
  }

  return 0;
}

int fromNetwork(tftp_packet *packet, int packetLength, char *buffer, int len) {
  if (packet == NULL || buffer == NULL || len < PACKET_LEN) {
    return -1;
  }
  memset(packet, 0, PACKET_LEN);
  int16_t code;
  memcpy(&code, buffer, sizeof(int16_t));
  //On met le code dans le paquet
  memcpy(&(packet->opCode), &code, sizeof(int16_t));
  switch(code) {
  case RRQ:
  case WRQ: {
    tftp_rwrq *pac = (tftp_rwrq *) packet;
    memcpy(pac->data, buffer + 2, RWR_DATA);
    memcpy(&(pac->mode), buffer + 2 + RWR_DATA, RWR_MODE);
    break;
  }
  case DATA: {
    tftp_data *pac = (tftp_data *) packet;
    memcpy(&(pac->blockNb), buffer + 2, 2);
    pac->datalen = packetLength - 4;
    memcpy(pac->data, buffer + 4, pac->datalen);
    break;
  }
  case ACK: {
    tftp_ack *pac = (tftp_ack *) packet;
    memcpy(&(pac->blockNb), buffer + 2, 2);
    break;
  }
  case ERROR: {
    tftp_error *pac = (tftp_error *) packet;
    memcpy(&(pac->errorCode), buffer + 2, 2);
    memcpy(pac->errMsg, buffer + 4, DATA_LEN);
  }
  default :
    return -1;
    break;
  }

  return 0;
}

int readPacket(tftp_packet *packet, SocketUDP *socket,
                char *address, int *port) {
  char buff[PACKET_LEN];
  int sizeRead = readFromSocketUDP(socket, buff, PACKET_LEN, address, port, -1, NULL);
  if (sizeRead == -1) {
    return -1;
  }
  //Construction du paquet
  if (fromNetwork(packet, sizeRead, buff, PACKET_LEN) == -1) {
    return -1;
  }
  return sizeRead;
}

int writePacket(tftp_packet *packet, SocketUDP *socket,
                  const char *address, int port) {
  char buff[PACKET_LEN];
  if (toNetwork(packet, buff, PACKET_LEN) == -1) {
    return -1;
  }
  return writeToSocketUDP(socket, address, port, buff, PACKET_LEN);
}
