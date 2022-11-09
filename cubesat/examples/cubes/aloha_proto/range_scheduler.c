#include "range_scheduler.h"

ipc_channel_t * channel;

struct timespec start;
int activemask = 0;
uint16_t distances[TOTAL_NODES];
coordinates_t node_abs_pos[TOTAL_NODES];

pthread_mutex_t coordinates_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t activemask_mutex = PTHREAD_MUTEX_INITIALIZER; // find a way to perform atomic operations

const node_id_t node_list[] = {NODE_1, NODE_2, NODE_3, NODE_4};

void stop_handler() {
	// This is a signal handler used to close
	// the shared memory file when CTRL+C is
	// pressed
	close_channel(channel); 
}

static void recv_scheduler() {
	node_id_t src;
	uint8_t buf[MAX_PAYLOAD_LEN];
	int recv_len = pop_recv_packet(buf, &src);

	if(recv_len == sizeof(coordinates_t)) {
		int nodeidx = 0;
		NODE_ID_TO_INDEX(src, nodeidx);
		
		// coordinates lock here are used to update absolute positions
		// of other nodes. This prevents error while exposing them
		// on a external API
		lock_coordinates
		
		memcpy(&(node_abs_pos[nodeidx]), buf, sizeof(coordinates_t));
		
		unlock_coordinates
	}
}

static void nbr_scheduler(nbr_t node, bool deleted) {
	
	int nodeidx = 0;
	NODE_ID_TO_INDEX(node.id, nodeidx);
	
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

int init_ranging_scheduler(char * serial_port) {
	clock_gettime(CLOCK_MONOTONIC_RAW, &start);

	if (!init(serial_port))
	{
		printf("Failed to initialize\n");
		return -1;
	}
	
	// Reset node coordinates memory
	memset(node_abs_pos, 0, TOTAL_NODES * sizeof(coordinates_t));
	memset(distances, 0, TOTAL_NODES * sizeof(uint16_t));
	
	// IPC //
	node_id_t my_id = get_node_id();
	if(start_channel(&channel, my_id) < 0) {
		printf("failed to initialize IPC\n");
		return -1;
	}
	
	fprintf(stderr, "channel size: %d\n", channel->shm_size);
	// INTRO //
	printintro(my_id, serial_port);

	init_callbacks();
	register_nbr_cb(nbr_scheduler);
	register_recv_cb(recv_scheduler);
	
	printf("\n\n\n");
	fflush(stdout);
	
	return 0;
}

int start_ranging_scheduler() {
	
	struct timespec monitor_before;
	struct timespec monitor_after;
	
	node_id_t my_id = get_node_id();
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
				
				coordinates_t tmpcoords;
				if(get_own_coords(channel, &tmpcoords, my_id) < 0) {
					dpprint("ERROR! cannot read IPC channel", get_node_id(), start);
					return -1;
				}
				
				// The only thread who can write this node absolute position
				// is the main thread so the lock is not necessary here to access
				// the data.				
				node_abs_pos[i].c_lat = tmpcoords.c_lat;
				node_abs_pos[i].c_lon = tmpcoords.c_lon;
				
				int broadcast_status = sync_broadcast((uint8_t *) &tmpcoords, sizeof(coordinates_t));
				if(broadcast_status == ERROR_TIMEOUT) {
					dpprint("send coordinates WARNING: timeout", get_node_id(), start);
				}
				else if(broadcast_status < 0) {
					dpprint("send coordinates error. error code: %d", get_node_id(), start, broadcast_status);
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
					dpprint("ERROR! node %X has a timeout while ranging", my_id, start, node_list[i]);
				}
				else if(ranging_status == ERROR_NOT_CALLABLE) {
					epprint("ERROR! Cannot perfom ranging with node %X", my_id, start, node_list[i]);
					// it should fail here, however there is a bug that make ranging work even with this error
					i++;
				}
				else{
					dpprint("ERROR! you shouldn't stay here node %X", my_id, start, node_list[i]);
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
		
		// MODIFY HERE TO WRITE THE DATA
		coordinates_t coords_tmp[TOTAL_NODES];
		
		lock_coordinates
		memcpy(coords_tmp, node_abs_pos, TOTAL_NODES * sizeof(coordinates_t));
		unlock_coordinates
		
		int mask;
		lock_active_mask;
		mask = activemask;
		unlock_active_mask;
		
		if(update_channel(channel, coords_tmp, distances, mask) < 0) {
			dpprint("ERROR! cannot write on IPC channel", get_node_id(), start);
			return -1;
		}
		
		msleep(abs(LOOP_DURATION - (int)rng_duration));
	}
	return 0;
}

