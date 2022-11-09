#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

#include "../wrapper/cubes_api.h"
#include "config.h"

#define RANGE_SUCCESS 1
#define RANGE_COLLISION 3

#define ERROR_NOT_CALLABLE -1
#define ERROR_TIMEOUT -2
#define ERROR_SYSTEM -3
#define ERROR_COLLISION -4

#define RANGING_TIMEOUT 400

#define BROADCAST_ADDRESS 0xFFFF

static void dist(nbr_t node, int status);

void init_callbacks();
int sync_ranging(node_id_t, uint16_t *);
int sync_broadcast(uint8_t * outbuff, int bufflen);
int sync_send(node_id_t dst, uint8_t * outbuff, int bufflen);
