#include "range_scheduler.h"

ipc_channel_t * channel;

struct timespec start;
int activemask = 0;
uint16_t adjmatrix[TOTAL_NODES * (TOTAL_NODES - 1) / 2];
coordinates_t node_abs_pos[TOTAL_NODES];
coordinates_t owncoord; // store coordinates that should be passed to the thread

// coordinates and active mask mutex
pthread_mutex_t coordinates_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t activemask_mutex = PTHREAD_MUTEX_INITIALIZER; // find a way to perform atomic operations
// adjacency matrix wait conditions
pthread_mutex_t adjm_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t adjm_cond = PTHREAD_COND_INITIALIZER;

node_id_t my_id;

int discovered_nodes = 1;

const node_id_t node_list[] = {NODE_1, NODE_2, NODE_3, NODE_4};

int heartbeat = 0;
char heratbeat_char[] = {'_', ' '};

void stop_handler() {
	// This is a signal handler used to close
	// the shared memory file when CTRL+C is
	// pressed.
	close_channel(channel); 
}

void snd_scheduler(node_id_t dst, uint16_t handle, enum tx_status status) 
{
	// we have no guarantee that this callback will be called by the broadcast.

	// unused parameters. Only one sync_send at time can be executed
	/*(void) handle;
	(void) dst;
	(void) status;

	if(pthread_mutex_lock(&adjm_mutex) < 0) {
		dpprint("Cannot get adjmatrix lock", my_id, start);
	}
	if(pthread_cond_signal(&adjm_cond) < 0) {
		return;
	}
	if(pthread_mutex_unlock(&adjm_mutex) < 0) {
		return;
	}*/
	return;
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
		pprint("neighbour node %X deleted", my_id, start, node.id);
		lock_active_mask
		discovered_nodes--;
		UNSET_BIT(activemask, nodeidx);
		unlock_active_mask
	}
	else {
		pprint("neighbour node %X discovered", my_id, start, node.id);
		lock_active_mask
		discovered_nodes++;
		SET_BIT(activemask, nodeidx);
		unlock_active_mask
	}
}

static void adjm_scheduler(adjm_descr_t* adjm)
{
	if(pthread_mutex_lock(&adjm_mutex) < 0) {
		dpprint("Cannot get adjmatrix lock", my_id, start);
	}
	
	/* TRANSLATION ZONE
	 * due to the request of the upper level, the following code has the purpose
	 * to provide the space for the entire adjacency matrix.
	 * However the symmetric parts and the diagonals (all 0s) are ignored.
	 * for example with 4 nodes the order is:
	 * 12, 13, 14, 
	 *     23, 24, 
	 *         34
	 * and the size of this vector is n * (n - 1) / 2
	 */
	 
	// Map the nodes receved into our order.
	// i.e. node_order_map[3] = 0 means that
	// NODE_4 is the first node of the received matrix
	int node_order_map[TOTAL_NODES] = {-1, -1, -1, -1};
	for(int i = 0; i < TOTAL_NODES; i++) {
		for(int j = 0; j < adjm->num_nodes; j++) {
			if(adjm->nodes[j] == node_list[i]) {
				node_order_map[i] = j;
			}
		}
	}
	
	// Then write the correct adjacency upper-triangular matrix
	int adjsize = TOTAL_NODES * (TOTAL_NODES - 1) / 2;
	int i1 = 0;
	int i2 = 1;
	for(int i = 0; i < adjsize; i++) {
		
		int matrix_node_x = node_order_map[i2];
		int matrix_node_y = node_order_map[i1];
		
		
		if(matrix_node_x == -1 || matrix_node_y == -1)
		 	// inactive node. The UNAVAILABLE_NODE macro is a 
		 	// wrapper for UINT16_MAX that is also used for 
		 	// failed matrix building
			adjmatrix[i] = UNAVAILABLE_NODE;
		else {
			int matrix_location = matrix_node_x * adjm->num_nodes + matrix_node_y;
			adjmatrix[i] = (adjm->matrix)[matrix_location].distance;
		}
		i2 = (i2 + 1) % TOTAL_NODES;
		if(i2 ==  0) {
			i1++;
			i2 = i1 + 1;
		}
	}
	
	// The node position is then sent in broadcast to all the other
	// nodes on the network.
	
	if(pthread_cond_signal(&adjm_cond) < 0) {
		dpprint("Cannot signal the adjmatrix cond variable", my_id, start);
	}
	
	if(pthread_mutex_unlock(&adjm_mutex) < 0) {
		dpprint("Cannot release adjmatrix lock", my_id, start);
	}
	
	// DUMB IDIOT! You cannot block this thread to achieve the synchronization of the broadcast.
	msleep(50);
	int broadcast_status = sync_broadcast((uint8_t *)  &(owncoord), sizeof(coordinates_t));
	if(broadcast_status == ERROR_TIMEOUT) {
		dpprint("send coordinates WARNING: timeout", my_id, start);
	}
	else if(broadcast_status != SYNC_SUCCESS) {
		dpprint("send coordinates error. error code: %d", my_id, start, broadcast_status);
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
	memset(adjmatrix, 0, (TOTAL_NODES * (TOTAL_NODES - 1) / 2) * sizeof(uint16_t));
	
	// IPC //
	my_id = get_node_id();
	if(start_channel(&channel, my_id) < 0) {
		printf("failed to initialize IPC\n");
		return -1;
	}
	
	// INTRO //
	printintro(my_id, serial_port);

	//init_callbacks();
	register_sent_cb(snd_scheduler);
	register_nbr_cb(nbr_scheduler);
	register_recv_cb(recv_scheduler);
	register_adjm_cb(adjm_scheduler);
	
	return 0;
}

int start_ranging_scheduler() {
	
	struct timespec monitor_before;
	struct timespec monitor_after;
	
	// if there is only 1 node, the adjm will fail. Until the first neighbor
	// callback is called, the adjmatrix function won't return success.
	int atomic_mask;
	int atomic_discovered;
	do {
		lock_active_mask
		atomic_discovered = discovered_nodes;
		atomic_mask = activemask;
		unlock_active_mask
		msleep(500);
		dpprint("No other nodes here. Waiting...", my_id, start);
	}
	while(atomic_discovered == 1);
	
	pprint("System ready. Starting protocol", my_id, start);
	
	while(1) {
		// register the initial time of the loop to compute
		// the sleeping time.
		clock_gettime(CLOCK_MONOTONIC_RAW, &monitor_before);
		
		/* RETRIVE OWN COORDINATES */
		// the coordinates should have been written by the kalman filter process
		// on the IPC channel. The get_own_coords() function will query the IPC
		// channel to obtain this data.
		// The only thread who can write this node absolute position
		// is the main thread so the lock is not necessary here to access
		// the data. Here we update the position with the one obtained
		// by the kalman filter through the IPC channel
		int myidx = 0;
		NODE_ID_TO_INDEX(my_id, myidx)	
	

		if(get_own_coords(channel, &(node_abs_pos[myidx]), my_id) < 0) {
			dpprint("ERROR! cannot read IPC channel", my_id, start);
			return -1;
		}
		
		/* BUILD ADJACENCY MATRIX */
		// get lock
		if(pthread_mutex_lock(&adjm_mutex) < 0) {
			dpprint("Cannot get adjmatrix lock", my_id, start);
		}
		
		// owncoord is synchronized inside the condition zone
		memcpy(&owncoord, &(node_abs_pos[myidx]), sizeof(coordinates_t));
		
		// if we are the master node, start the procedure
		if(my_id == NODE_MASTER) {
			if(!build_adjm(NULL, 4)) {
				dpprint("ERROR! adjacency matrix call failed", my_id, start);
				if(pthread_mutex_unlock(&adjm_mutex) < 0) {
					dpprint("Cannot release adjmatrix lock", my_id, start);
				}
				// If the adjacency matrix fails, a shorter time is waited before
				// retrying. Each other procedure is skipped.
				msleep(WAIT_BEFORE_RETRY_ADJM);
				continue;
			}
			struct timespec timeout;
			clock_gettime(CLOCK_REALTIME, &timeout);
			timespec_add_msec(timeout, ADJM_TIMEOUT);
			// in either cases wait for a callback
			if(pthread_cond_timedwait(&adjm_cond, &adjm_mutex, &timeout) < 0) {
				dpprint("Pthread cond wait failed on adjm condition variable", my_id, start);
			}
		}
		else {
			if(pthread_cond_wait(&adjm_cond, &adjm_mutex) < 0) {
				dpprint("Pthread cond wait failed on adjm condition variable", my_id, start);
			}
		}
		// release lock
		if(pthread_mutex_unlock(&adjm_mutex) < 0) {
			dpprint("Cannot release adjmatrix lock", my_id, start);
		}
		
		
		/* RECOVER RECEIVED COORDINATES */
		// In order to avoid locking the recv callback, this software won't
		// update the ipc directly through the callback. Instead after each
		// round the received coordinate vector is copied in the channel
		// entirely. The temporary vector is used to avoid deathlocks.
		coordinates_t coords_tmp[TOTAL_NODES];
		
		lock_coordinates
		memcpy(coords_tmp, node_abs_pos, TOTAL_NODES * sizeof(coordinates_t));
		unlock_coordinates
		
		
		if(update_channel(channel, coords_tmp, adjmatrix, atomic_mask, my_id) < 0) {
			dpprint("ERROR! cannot write on IPC channel", my_id, start);
			return -1;
		}
		
		
		// Only the master node should sleep the LOOP_DURATION due to the fact that
		// the other nodes are stuck in the pthread cond wait. If the other nodes are
		// sleeping too, the callback could interrupt the sleeping and then they wait for the
		// next condition ignoring the first one.
		if(my_id == NODE_MASTER) {
			/* wait for the next round */
			// compute the sleeping time
			clock_gettime(CLOCK_MONOTONIC_RAW, &monitor_after);
			double rng_duration = timespec_to_msec(monitor_after, monitor_before);
			int sleepduration = LOOP_DURATION - (int) rng_duration;
			if(sleepduration < 1)
				sleepduration = 1;
			// wait for at least 1ms if rng_duration >= LOOP_DURATION
			msleep(sleepduration);
			
			// Print the heartbeat
			heartbeat = (heartbeat + 1) % 2;
			pprint("%c     duration: %.2f               ", my_id, start, heratbeat_char[heartbeat], rng_duration);
		}
		else {
			// the other nodes don't need to monito loop duration
			heartbeat = (heartbeat + 1) % 2;
			pprint("%c                                  ", my_id, start, heratbeat_char[heartbeat]);
		}
	}
	return 0;
}

