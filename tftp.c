#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <arpa/inet.h>

#include "tftp.h"
#include "socketUDP.h"
#include "commons.h"

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
        int flength, const char *file, int mlength, const char *mode) {
  if (flength > RWR_DATA - 1 || mlength > RWR_MODE - 1) {
    return -1;
  }
  if (code != RRQ && code != WRQ) {
    return -1;
  }
  if (packet == NULL) {
    return -1;
  }
  packet->opCode = code;
  memset(packet->data, 0, RWR_DATA);
  memset(packet->mode, 0, RWR_MODE);
  strncpy(packet->data, file, flength);
  strncpy(packet->mode, mode, mlength);
  return 0;
}

int createDATA(tftp_data *packet, int nb, int datalen, char *data) {
  if (datalen > DATA_LEN) {
    return -1;
  }
  if (packet == NULL) {
    return -1;
  }
  packet->opCode = DATA;
  packet->blockNb = nb;
  packet->datalen = datalen;
  memset(packet->data, 0, datalen);
  memcpy(packet->data, data, datalen);
  return 0;
}

int createACK(tftp_ack *packet, int nb) {
  if (packet == NULL) {
    return -1;
  }
  packet->opCode = ACK;
  packet->blockNb = nb;

  return 0;
}

int createERR(tftp_error *packet, int errCode,
        int messlen, const char *message) {
  if (messlen > DATA_LEN - 1) {
    return -1;
  }
  if (packet == NULL) {
    return -1;
  }
  memset(packet->errMsg, 0, DATA_LEN);
  packet->opCode = ERROR;
  packet->errorCode = errCode;
  strncpy(packet->errMsg, message, messlen);

  return 0;
}

/*
 * Communication.
 */
int sendAndWait(tftp_packet *send, tftp_packet *wait,
        int timeout, SocketUDP *s,
        const char *address, int port, int *endTime,
        char *srcAddr, int *srcPort) {

  
  char netPacket[PACKET_LEN];
  int dataBlocNb = -1;
  int lengthToWrite = PACKET_LEN;
  if (send->opCode == DATA) {
    tftp_data *data = (tftp_data *) send;
    dataBlocNb = data->blockNb;
    lengthToWrite = data->datalen + 4;
  }
  if (toNetwork(send, netPacket, PACKET_LEN) == -1) {
    printf("1");
    return -1;
  }
  int writed;
  if ((writed = writeToSocketUDP(s, address, port, netPacket, lengthToWrite)) == -1) {
    return -1;
  }
  if (writed != lengthToWrite) {
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
    memcpy(endTime, &time, sizeof (int));
    return -1;
  }
  
  memcpy(endTime, &time, sizeof (int));
  if (time == 0) {
    return -1;
  }
  
  if (fromNetwork(&tmp, PACKET_LEN, buff, packetLen) == -1) {
    return -1;
  }
  
//  printf("tmp.opCode != ERROR : %d\n", tmp.opCode != ERROR);
  if ((tmp.opCode != PACK_WAIT[send->opCode]) && (tmp.opCode != ERROR)) {
    return -1;
  }
  if (dataBlocNb != -1) {
    //Le paquet de départ était un Data, on a passé PACK_WAIT[], on
    //a donc ici un ACK. Il faut donc tester le numero de bloc
    tftp_ack *ack = (tftp_ack *) &tmp;
    if (ack->blockNb != dataBlocNb) {
      printf("8");
      return -1;
    }
  }
  if (send->opCode != RRQ && send->opCode != WRQ) {
    if ((strcmp(addr, address) != 0) || port != senderPort) {
      tftp_error errPac;
      char *msg = "Unexpected address and port";
      createERR(&errPac, 5, strlen(msg), msg);
      toNetwork((tftp_packet *) & errPac, buff, PACKET_LEN);
      writeToSocketUDP(s, addr, senderPort, buff, PACKET_LEN);
      return -2;
    }
  }
  
  memcpy(&(wait->opCode), &(tmp.opCode), 2);  
  memcpy(wait->data, tmp.data, PACKET_LEN + 2);  
  
  if (srcAddr != NULL) {
    strncpy(srcAddr, addr, SHORT_BUFFER_LEN);
  }
  if (srcPort != NULL) {
    *srcPort = senderPort;
  }
  return packetLen;
}

int sendLoop(tftp_packet* toSend, tftp_packet* toReceive,
        int timeout, SocketUDP* s, const char* address,
        int port) {
  int res = -1;
  for (int i = 0; (i < SEND_LOOP_NB) && (res <= 0); i++) {
    int endTime;
    res = sendAndWait(toSend, toReceive, timeout, s,
            address, port, &endTime, NULL, NULL);
    //Si res == 0, on sortira de la boucle, ce sera terminé.
    //Si res == -1, la boucle recommencera pour le prochain essai
    //Si res == -2, on relance cet essai en diminuant le timeout
    while (res == -2) {
      printf("TFTP: res == -2 / retrying with timeout=%d", endTime);
      int newTimeout = endTime;
      res = sendAndWait(toSend, toReceive, newTimeout, s,
                        address, port, &endTime, NULL, NULL);
    }
  }
  return res;
}

int toNetwork(tftp_packet *packet, char *buffer, int len) {
  if (packet == NULL || buffer == NULL || len < PACKET_LEN) {
    return -1;
  }
  memset(buffer, 0, len);
  uint16_t netCode = htons(packet->opCode);
  memcpy(buffer, &netCode, 2);
  switch (packet->opCode) {
    case RRQ:
    case WRQ:
    {
      tftp_rwrq *pac = (tftp_rwrq *) packet;
      memcpy(buffer + 2, pac->data, RWR_DATA);
      memcpy(buffer + 2 + RWR_DATA, pac->mode, RWR_MODE);
      break;
    }
    case DATA:
    {
      tftp_data *pac = (tftp_data *) packet;
      uint16_t netBlockNb = htons(pac->blockNb);
      memcpy(buffer + 2, &netBlockNb, 2);
      memcpy(buffer + 4, pac->data, pac->datalen);
      break;
    }
    case ACK:
    {
      tftp_ack *pac = (tftp_ack *) packet;
      uint16_t netBlockNb = htons(pac->blockNb);
      memcpy(buffer + 2, &netBlockNb, 2);
      break;
    }
    case ERROR:
    {
      tftp_error *pac = (tftp_error *) packet;
      uint16_t netErrCode = htons(pac->errorCode);
      memcpy(buffer + 2, &netErrCode, 2);
      memcpy(buffer + 4, pac->errMsg, DATA_LEN);
      break;
    }
    default:
      return -1;
      break;
  }

  return 0;
}

int fromNetwork(tftp_packet *packet, int packetLength, char *buffer, int len) {
  if (packet == NULL || buffer == NULL || len > PACKET_LEN) {
    return -1;
  }
  memset(packet, 0, packetLength);
  int16_t code;
  memcpy(&code, buffer, sizeof (int16_t));
  code = ntohs(code);
  //On met le code dans le paquet
  memcpy(&(packet->opCode), &code, sizeof (int16_t));
  switch (code) {
    case RRQ:
    case WRQ:
    {
      tftp_rwrq *pac = (tftp_rwrq *) packet;
      memcpy(pac->data, buffer + 2, RWR_DATA);
      memcpy(pac->mode, buffer + 2 + RWR_DATA, RWR_MODE);
      break;
    }
    case DATA:
    {
      tftp_data *pac = (tftp_data *) packet;
      memcpy(&(pac->blockNb), buffer + 2, 2);
      pac->datalen = len - 4;
      memcpy(pac->data, buffer + 4, pac->datalen);
      //"Déréseautisation"
      pac->blockNb = ntohs(pac->blockNb);
      break;
    }
    case ACK:
    {
      tftp_ack *pac = (tftp_ack *) packet;
      memcpy(&(pac->blockNb), buffer + 2, 2);
      //"Déréseautisation"
      pac->blockNb = ntohs(pac->blockNb);
      break;
    }
    case ERROR:
    {
      tftp_error *pac = (tftp_error *) packet;
      memcpy(&(pac->errorCode), buffer + 2, 2);
      memcpy(pac->errMsg, buffer + 4, DATA_LEN);
      //"Déréseautisation"
      pac->errorCode = ntohs(pac->errorCode);
      break;
    }
    default:
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
  if (fromNetwork(packet, PACKET_LEN, buff, sizeRead) == -1) {
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
