#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include "./pretty_printer.h"
#include "./network_handler.h"
#include "./config.h"
#include "../wrapper/cubes_api.h"

struct timespec start;
node_id_t node_list[TOTAL_NODES] = {NODE_1, NODE_2, NODE_3, NODE_4};
int activemask = 0;
uint16_t distances[TOTAL_NODES];
coordinates_t node_abs_pos[TOTAL_NODES];

pthread_mutex_t coordinates_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t activemask_mutex = PTHREAD_MUTEX_INITIALIZER; // find a way to perform atomic operations

static void recv() {
	node_id_t src;
	uint8_t buf[MAX_PAYLOAD_LEN];
	int recv_len = pop_recv_packet(buf, &src);
	//pprint("coordinates received from %x, len %d\n", get_node_id(), start, src, recv_len);
	if(recv_len == sizeof(coordinates_t)) {
		int nodeidx;
		for(int i = 0; i < TOTAL_NODES; i++) {
			if(node_list[i] == src) {
				nodeidx = i;
			}
		}
		
		// coordinates lock here are used to update absolute positions
		// of other nodes. This prevents error while exposing them
		// on a external API
		lock_coordinates
		
		memcpy(&(node_abs_pos[nodeidx]), buf, sizeof(coordinates_t));
		
		unlock_coordinates
	}
}

static void nbr(nbr_t node, bool deleted) {
	
	int nodeidx = 0;
	for(int i = 0; i < TOTAL_NODES; i++) {
		if(node_list[i] == node.id) {
			nodeidx = i;
		}
	}
	
	if(deleted) {
		pprint("neighbour node %X deleted", get_node_id(), start, node.id);
		lock_active_mask
		UNSET_BIT(activemask, nodeidx);
		unlock_active_mask
	}
	else {
		pprint("neighbour node %X discovered", get_node_id(), start, node.id);
		lock_active_mask
		SET_BIT(activemask, nodeidx);
		unlock_active_mask
	}
}


int main(int argc, char *argv[]) {
	struct timespec monitor_before;
	struct timespec monitor_after;
	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
	
	// Software init
	if (argc < 2) {
		printf("Specify the device file name\n");
		return 1;
	}

	if (!init(argv[1]))
	{
		printf("Failed to initialise\n");
		return 0;
	}
	
	// Reset node coordinates memory
	memset(node_abs_pos, 0, TOTAL_NODES * sizeof(coordinates_t));
	memset(distances, 0, TOTAL_NODES * sizeof(uint16_t));
	
	// INTRO //
	node_id_t my_id = get_node_id();
	printintro(my_id, argv[1]);

	init_callbacks();
	register_nbr_cb(nbr);
	register_recv_cb(recv);
	
	printf("\n\n\n");
	fflush(stdout);
	
	while(1) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &monitor_before);
		int i = 0;
		//int collision_counter = 0;
		while(i < TOTAL_NODES) {
			lock_active_mask
			if(node_list[i] == my_id) {
				unlock_active_mask
				// Here I can broadcast my absolute position

				distances[i] = 0;

				// The only thread who can write this node absolute position
				// is the main thread so the lock is not necessary here to access
				// the data.
				int broadcast_status = sync_broadcast((uint8_t *) &(node_abs_pos[i]), sizeof(coordinates_t));
				if(broadcast_status == ERROR_TIMEOUT) {
					epprint("send coordinates WARGING: timeout", get_node_id(), start);
				}
				else if(broadcast_status < 0) {
					epprint("send coordinates error. error code: %d", get_node_id(), start, broadcast_status);
					//return -1;
				}
				i++;
			}
			else if(GET_BIT(activemask, i)) {
				unlock_active_mask
				// Active node: range to be checked
				int ranging_status = sync_ranging(node_list[i], distances + i);
				if(ranging_status == 0) {
					i++;
				}
				else if(ranging_status == ERROR_NO_RESPONSE || ranging_status == ERROR_TIMEOUT) {
					// This error may happen in three different situation:
					// 1. if the ranging procedure has a collision
					// 2. if the target node is not online anymore or is out of range
					// 3. if there is a packet lost during ranging.
					// There is no distinctive code for those cases so we assume that if
					// the target node is not callable for more than COLLISION_COUNTER_LIMIT
					// times, then the node is offline. 
					msleep(rand() % MAX_WAIT_BEFORE_RETRY);
					/*collision_counter++;
					if(collision_counter > COLLISION_COUNTER_LIMIT){
						lock_active_mask
						UNSET_BIT(activemask, i);
						unlock_active_mask
						epprint("ERROR! node %X has a timeout while ranging", my_id, start, node_list[i]);
						i++;
					}*/
					epprint("ERROR! node %X has a timeout while ranging", my_id, start, node_list[i]);
				}
				else if(ranging_status == ERROR_NOT_CALLABLE) {
					epprint("ERROR! Cannot perfom ranging with node %X", my_id, start, node_list[i]);
					// it should fail here, however there is a bug that make ranging work even with this error
					i++;
				}
				else{
					epprint("ERROR! you shouldn't stay here node %X", my_id, start, node_list[i]);
					return -1;
				}
			}
			else {
				unlock_active_mask
				// Inactive node: ignore
				i++;
			}
		}
		clock_gettime(CLOCK_MONOTONIC_RAW, &monitor_after);
		double rng_duration = timespec_to_msec(monitor_after, monitor_before);
		
		msleep(abs(LOOP_DURATION - (int)rng_duration));
		
		clock_gettime(CLOCK_MONOTONIC_RAW, &monitor_after);
		double loop_duration = timespec_to_msec(monitor_after, monitor_before);
		
		
		coordinates_t print_tmp[TOTAL_NODES];
		
		lock_coordinates
		memcpy(print_tmp, node_abs_pos, TOTAL_NODES * sizeof(coordinates_t));
		unlock_coordinates
		
 		printall(distances, activemask, node_list, print_tmp, rng_duration, loop_duration);
	}

	return 0;
}
