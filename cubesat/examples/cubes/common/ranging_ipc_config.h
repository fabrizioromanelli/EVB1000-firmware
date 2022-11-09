#pragma once

#include "common_config.h"
#include "../wrapper/cubes_api.h"

// Suppose that
//	NODE_1 -> ROVER1
//	NODE_2 -> ROVER2
//	NODE_3 -> ROVER3
//	NODE_4 -> QUADCOPTER
// Then change the addresses accordingly.

#define NODE_1 0x45bb
#define NODE_2 0x4890
#define NODE_3 0x0C16
#define NODE_4 0x4829

#define TOTAL_NODES 4
#if TOTAL_NODES > 8
	#error "Max available nodes are 8"
#endif

extern const node_id_t node_list[];

#define LOOP_DURATION 500 //msec
#define MAX_WAIT_BEFORE_RETRY_ADJM 100 //msec
#define ADJM_TIMEOUT 300 //msec

#define lock_coordinates \
		if(pthread_mutex_lock(&coordinates_mutex) < 0) {\
			epprint("Error while acquiring coordinates_mutex", get_node_id(), start);\
			exit(-1);\
		}
		
#define unlock_coordinates\
		if(pthread_mutex_unlock(&coordinates_mutex) < 0) {\
			epprint("Error while releasing coordinates_mutex", get_node_id(), start);\
			exit(-1);\
		}

#define lock_active_mask \
		if(pthread_mutex_lock(&activemask_mutex) < 0) {\
			epprint("Error while acquiring activemask_mutex", get_node_id(), start);\
			exit(-1);\
		}
		
#define unlock_active_mask\
		if(pthread_mutex_unlock(&activemask_mutex) < 0) {\
			epprint("Error while releasing activemask_mutex", get_node_id(), start);\
			exit(-1);\
		}

#define NODE_ID_TO_INDEX(node_id, outindex) \
	for(int i = 0; i < TOTAL_NODES; i++) {\
		if(node_list[i] == (node_id)) {\
			outindex = i;\
		}\
	}
	
