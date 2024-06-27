#ifndef TLV_H
#define TLV_H
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  uint32_t userID;
  int actionID;
  int seatNumber;
  char *password;
} Query;

typedef struct {
  int actionID;
  bool seats[256 + 1];
  int code;
} Res;
typedef struct {
  uint32_t type;
  uint32_t length;
  uint8_t *value;
} tlv_t;

int recv_tlv(int fd, tlv_t *tlv);
void send_tlv(int fd, tlv_t *tlv);
#endif