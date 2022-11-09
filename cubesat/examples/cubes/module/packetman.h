/*---------------------------------------------------------------------------*/
#ifndef PACKETMAN_H
#define PACKETMAN_H
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
#include "core/net/linkaddr.h"
/*---------------------------------------------------------------------------*/
/* get and remove from buffer (used upon receipt) */
int packetman_extract(void *copy, size_t copy_size);
/*---------------------------------------------------------------------------*/
/* add header before payload (used upon message preparation) */
int packetman_sethdr(void *header, size_t header_size);
/*---------------------------------------------------------------------------*/
/* add to payload (used upon message preparation) */
int packetman_setpayload(void *payload, size_t payload_size);
/*---------------------------------------------------------------------------*/
#endif /*PACKETMAN_H */
/*---------------------------------------------------------------------------*/
