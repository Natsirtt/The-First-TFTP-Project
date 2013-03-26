#ifndef TFTP_H
#define	TFTP_H

#include <sys/types.h>

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
  char data[498];
  char mode[16];
} tftp_rwrq;

typedef struct {
  int16_t opCode;
  int16_t blockNb;
  char data[512];
} tftp_data;

typedef struct {
  int16_t opCode;
  int16_t errorCode;
  char errMsg[512];
} tftp_error;

#endif	/* TFTP_H */
