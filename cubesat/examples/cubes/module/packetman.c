#include <stdbool.h>
#include "contiki.h"
#include "net/rime/rime.h"
#include "net/netstack.h"
#include <stdio.h>
#include "core/net/linkaddr.h"

#include "packetman.h"

#define DEBUG 0

/*---------------------------------------------------------------------------*/
/* get and remove from buffer (used upon receipt) */
int packetman_extract(void *copy, size_t copy_size) {
  memcpy(copy, packetbuf_dataptr(), copy_size);
  if(!packetbuf_hdrreduce(copy_size)) {
#if DEBUG
    printf("packetman: WARNING. No header to reduce.\n");
#endif
    return 0;
  }

	return 1;
}
/*---------------------------------------------------------------------------*/
/* add header before payload (used upon message preparation) */
int packetman_sethdr(void *header, size_t header_size) {
  if(!packetbuf_hdralloc(header_size)) {
#if DEBUG
    printf("packetman: WARNING. No header space.\n");
#endif
    return 0;
  }
  memcpy(packetbuf_hdrptr(), header, header_size);

	return 1;
}
/*---------------------------------------------------------------------------*/
/* add to payload (used upon message preparation) */
int packetman_setpayload(void *payload, size_t payload_size) {
  if(packetbuf_totlen() + payload_size > PACKETBUF_SIZE) {
#if DEBUG
    printf("packetman: WARNING. No payload space.\n");
#endif
    return 0;
  }
  memcpy(packetbuf_dataptr() + packetbuf_datalen(), payload, payload_size);
  packetbuf_set_datalen(packetbuf_datalen() + payload_size);

	return 1;
}
/*---------------------------------------------------------------------------*/
