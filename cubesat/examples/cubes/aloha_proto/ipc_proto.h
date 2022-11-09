#pragma once

#include <string.h>

#include "../common/ranging_ipc_config.h"
#include "../wrapper/cubes_api.h"
#include "../common/ipc_interface.h"

#define IPC_SIZE (TOTAL_NODES * (sizeof(uint16_t) + sizeof(coordinates_t)) + sizeof(uint64_t) + sizeof(uint16_t))
#define CONST_SIZE ((TOTAL_NODES + 2) * sizeof(int))

#define DIST_LIST_OFFSET 0
#define COORDS_LIST_OFFSET (DIST_LIST_OFFSET + TOTAL_NODES * sizeof(uint16_t))
#define TIMESTAMP_OFFSET (COORDS_LIST_OFFSET + TOTAL_NODES * sizeof(coordinates_t))
#define ACTIVE_MASK_OFFSET (TIMESTAMP_OFFSET + sizeof(uint64_t))

#define NUM_OF_NODES_OFFSET 0
#define CURRENT_NODE_ID_OFFSET (NUM_OF_NODES_OFFSET + sizeof(uint16_t))
#define NODE_LIST_OFFSET (CURRENT_NODE_ID_OFFSET + sizeof(uint16_t))

void close_channel(ipc_channel_t * c);
int start_channel(ipc_channel_t ** c, node_id_t my_id);
int update_channel(ipc_channel_t * c, coordinates_t * coord_list, uint16_t * distance_list, uint16_t activebitmap);
int get_own_coords(ipc_channel_t * c, coordinates_t * coords, node_id_t my_id);
