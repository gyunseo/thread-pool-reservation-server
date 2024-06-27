#include "tlv.h"
int recv_tlv(int fd, tlv_t *tlv) {
  uint8_t header[sizeof(tlv->type) + sizeof(tlv->length)];
  if (read(fd, header, sizeof(header)) == 0) {
    return -1;
  }
  memcpy(&(tlv->type), header, sizeof(tlv->type));
  memcpy(&(tlv->length), header + sizeof(tlv->type), sizeof(tlv->length));
  if (tlv->length == 0) {
    tlv->value = NULL;
    return 0;
  }
  tlv->value = (uint8_t *)malloc(tlv->length); // Cast to uint8_t*
  if (read(fd, tlv->value, tlv->length) == 0) {
    return -1;
  }
  return 0;
}
void send_tlv(int fd, tlv_t *tlv) {
  size_t total_size = sizeof(tlv->type) + sizeof(tlv->length) + tlv->length;
  uint8_t *buffer = malloc(total_size);
  memcpy(buffer, &(tlv->type), sizeof(tlv->type));
  memcpy(buffer + sizeof(tlv->type), &(tlv->length), sizeof(tlv->length));
  memcpy(buffer + sizeof(tlv->type) + sizeof(tlv->length), tlv->value,
         tlv->length);
  write(fd, buffer, total_size);
  free(buffer);
}
