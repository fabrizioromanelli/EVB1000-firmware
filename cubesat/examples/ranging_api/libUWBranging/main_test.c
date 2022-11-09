#include <stdio.h>
#include <stdlib.h>

#include "ranging_api.h"
#include "../common/pretty_printer.h"

/***************************
 * DEMO:
 * this file is intended to provide an example of how to use the ranging APIs.
 * In order to start the system, run the ranging_test executable in the
 * aloha_proto directory by passing the device file of module serial port
 * (i.e. ./ranging_test /dev/ttyACM0)
 *
 * In this way three files are created inside the shmfs: 
 * telemetry_const, telemetry_shm, sem.telemetrysem
 *
 * These files represents a POSIX IPC channel and the relative semaphore. However
 * the API provided here should garant that you doesn't need to read directly
 * the files.
 *
 * IPC files are created only by the ranging_api process.
 * Moreover these files are created in exclusive mode, so
 * no ranging process can begin if the files exists.
 *
 * Please check and delete them if they already exists before starting the
 * ranging executable.
 */


int main(int argc, char ** argv) {
	(void) argc;
	(void) argv;
	
	/* 
	 * If the start_ipc() fails, probably the ranging process
	 * is not in execution. Wait for it for a bit and then fail
	 */
	int ipc_test_counter = 0;
	ipc_channel_t * channel = NULL;
	do {
		channel = start_ipc();
		if(channel == NULL) {
			fprintf(stderr, "IPC not ready.\n");
			if(ipc_test_counter < 5){
				fprintf(stderr, "retry\n");
			}
			else {
				fprintf(stderr, "maximum attempt reached\n");
				exit(1);
			}
			ipc_test_counter++;
			usleep(500000);
		}
	} while(channel == NULL);
	
	/* Here the channel has been opened. We can begin reading
	 * constants and configure all we need to print data.
	 */
	
	// number of nodes
	int number_of_nodes = get_number_of_nodes(channel);
	if(number_of_nodes < 0) {
		fprintf(stderr, "Error, invalid number of nodes\n");
		exit(1);
	}
	
	// nodes list
	uint16_t * nodes_list = malloc(number_of_nodes * sizeof(uint16_t));
	if(nodes_list == NULL) {
		fprintf(stderr, "memory allocation error\n");
		exit(1);
	}
	if(get_node_list(channel, nodes_list) < 0) {
		fprintf(stderr, "Error, invalid node list\n");
		exit(1);
	}
	
	// my id
	uint16_t my_id = get_my_nodeid(channel);
	if(my_id == 0) {
		fprintf(stderr, "Error, invalid ID\n");
		exit(1);
	}
	
	/* We have read all the constant information we need.
	 * here we can start our protocol
	 */
	printintro(my_id, "__SHM_FILE__ /telemetry");
	fprintf(stdout, "\n\n\n\n");
	fflush(stdout);

	/* vector allocation: here adjacency vector and coordinates
	 * vector are preallocated to be filled in the main loop
	 */
	uint16_t * adjacency_vector = malloc(number_of_nodes * (number_of_nodes - 1) /  2 * sizeof(uint16_t));
	if(adjacency_vector == NULL) {
		fprintf(stdout, "OutOfMemoryError: cannot allocate adjacency vector\n");
		exit(1);
	}
	coordinates_t * coords_vector = malloc(number_of_nodes * sizeof(coordinates_t));
	if(adjacency_vector == NULL) {
		fprintf(stdout, "OutOfMemoryError: cannot allocate adjacency vector\n");
		exit(1);
	}
	uint64_t timestamp;
	
	double lat_demo = 0.0;
	double lon_demo = 0.0;
	
	
	/* MAIN LOOP ************************************/
	while(1) {
		/* each round get_data_vector() must be called. it
		 * doesn't matter how much time passes between two
		 * queries because the data is always available and
		 * always protected by the semaphore.
		 * However the timestamp value should be used as a 
		 * reference to check if new data is available.
		 */
		if(get_data_vectors(channel, adjacency_vector, coords_vector, &timestamp) < 0) {
			fprintf(stderr, "ERROR! cannot read shared memory\n");
			exit(1);
		}
		else {
			printall(adjacency_vector, 0, nodes_list, coords_vector, 0, 0);
		}
		
		if(write_coordinates(channel, lat_demo, lon_demo) < 0) {
			fprintf(stderr, "ERROR! cannot write on shared memory\n");
			exit(1);
		}
		
		lat_demo = lat_demo + 1;
		lon_demo = lon_demo + 1;
		usleep(500000);
	}
	
	exit(0);
	
}
