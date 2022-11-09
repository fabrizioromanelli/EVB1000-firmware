#include "cubes_api.h"
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

static void recv() {
	node_id_t src;
	uint8_t buf[MAX_PAYLOAD_LEN];
	int recv_len = pop_recv_packet(buf, &src);
	printf("[NODE %X] recv from %x, len %d\n", get_node_id(), src, recv_len);
}

static void nbr(nbr_t node, bool status) {
	printf("[NODE %X] nbr cb for node %X, %d\n", get_node_id(), node.id, status);
}

static void rall(rall_descr_t* rall) {
	int i;

	printf("[NODE %X] rall cb for init %X\n", get_node_id(), rall->init);
	for (i = 0; i < rall->num_nodes; i++) {
		printf("\t%X:%d", rall->nodes[i], rall->distances[i]);
	}
}

static void adjm(adjm_descr_t* adjm) {

	int i, j;
	int matrix_location;

	printf("[NODE %X] adjm cb\n", get_node_id());
	for (i = 0; i < adjm->num_nodes; i++) {
		printf("\t%X", adjm->nodes[i]);
			for (j = 0; j < adjm->num_nodes; j++) {
			matrix_location = i * adjm->num_nodes + j;
			printf("\t%X:%d", adjm->nodes[j], (adjm->matrix)[matrix_location].distance);
		}
		printf("\n");
	}
	
	usleep(50000);
	
	if(send(0, "DEMO", 5, 11) != OQ_SUCCESS) {
		printf("SEND FAILED\n");
	}
}

int main(int argc, char *argv[]) {

	if (argc<2) {
		printf("Specify the device file name\n");
		return 1;
	}

	if(!init(argv[1])) {
		printf("init failed\n");
		return -1;
	}

	// in this test the module is used as a slave only

	register_sent_cb(NULL);
	register_recv_cb(recv);
	register_rng_cb(NULL);
	register_nbr_cb(nbr);
	register_rall_cb(rall);
	register_adjm_cb(adjm);


	while (1) {
		sleep(1);
	}	
	return 0;
}
