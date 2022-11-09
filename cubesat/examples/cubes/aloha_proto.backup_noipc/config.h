#pragma once

#define NODE_1 0x45bb
#define NODE_2 0x4890
#define NODE_3 0x0C16
#define NODE_4 0x4829

#define TOTAL_NODES 4

#define LOOP_DURATION 500 //msec
#define MAX_WAIT_BEFORE_RETRY 20 //msec
#define COLLISION_COUNTER_LIMIT 3

#if TOTAL_NODES > 32
	#error "Max available nodes are 32"
#endif

// Timing definitions
#define timespec_to_sec(t1, t2) ((double)(t1.tv_nsec - t2.tv_nsec) * 1e-9 + (double)(t1.tv_sec - t2.tv_sec))
#define timespec_to_msec(t1, t2) ((double)(t1.tv_nsec - t2.tv_nsec) * 1e-6 + (double)(t1.tv_sec - t2.tv_sec) * 1e3)

#define timespec_add_msec(ts, msec) uint64_t adj_nsec = 1000000ULL * msec;\
ts.tv_nsec += adj_nsec;\
if (adj_nsec >= 1000000000) {\
	uint64_t adj_sec = adj_nsec / 1000000000;\
	ts.tv_nsec -= adj_sec * 1000000000;\
	ts.tv_sec += adj_sec;\
}\
if (ts.tv_nsec >= 1000000000){\
	ts.tv_nsec -= 1000000000;\
	ts.tv_sec++;\
}

#define msleep(msec) usleep((msec) * 1000)

// utilities
#define UNSET_BIT(mask, index) mask &= ~(1 << (index))
#define SET_BIT(mask, index) mask |= 1 << (index)
#define GET_BIT(mask, index) mask & (1 << index)

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

// structures
typedef struct coordinates {
	double c_lat;
	double c_lon;
	double c_h;
} coordinates_t;
