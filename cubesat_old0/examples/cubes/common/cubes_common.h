#ifndef CUBES_COMMON_H
#define CUBES_COMMON_H
#include <stdint.h>


// max length of a packet (for both data and control packets)
#define MAX_MODULE_PACKET_LEN 128

int decode_hex(const char* input, uint16_t len, char* decoded);
int encode_hex(const char* input, uint16_t len, char* encoded);

#define htons(A) ((((uint16_t)(A) & 0xff00) >> 8) | \
                  (((uint16_t)(A) & 0x00ff) << 8))
#define htonl(A) ((((uint32_t)(A) & 0xff000000) >> 24) | \
                  (((uint32_t)(A) & 0x00ff0000) >> 8)  | \
                  (((uint32_t)(A) & 0x0000ff00) << 8)  | \
                  (((uint32_t)(A) & 0x000000ff) << 24))

#define ntohs  htons
#define ntohl  htonl


#endif
