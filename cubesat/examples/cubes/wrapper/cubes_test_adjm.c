#include "cubes_api.h"
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#define timespec_to_msec(t1, t2) ((double)(t1.tv_nsec - t2.tv_nsec) * 1e-6 + (double)(t1.tv_sec - t2.tv_sec) * 1e3)

struct timespec monitor_before;
struct timespec monitor_after;
char vector_test[7] = {'a','b','c','d','e','f','g'};

volatile bool go_on;

static void sent(node_id_t dst, uint16_t handle, enum tx_status status) {
  clock_gettime(CLOCK_MONOTONIC_RAW, &monitor_after);
  double d = timespec_to_msec(monitor_after, monitor_before);
  printf("Sent duration: %.2f,  %X, %d, %d\n", d, dst, handle, status);
}

static void dist(nbr_t node, int status) {
  printf("App: distance %X, %d, %d\n", node.id, status, node.distance);
}

static void recv() {
  node_id_t src;
  uint8_t buf[MAX_PAYLOAD_LEN];
  int recv_len = pop_recv_packet(buf, &src);
  printf("App: recv from %x, len %d\n", src, recv_len);
}

static void nbr(nbr_t node, bool status) {
  printf("App: nbr cb for node %X, %d\n", node.id, status);

  int nbr_i;
  nbr_t nbr;
  nbr_i = nbr_table_next(-1, &nbr);
  printf("NBR_I = %d\n", nbr_i);
  if(nbr_i >= 0) printf("Neighbor %d: %04X, ts %d\n", nbr_i, nbr.id, nbr.last_heard);
  while(nbr_i != -1) {
    printf("NBR_I = %d\n", nbr_i);
    nbr_i = nbr_table_next(nbr_i, &nbr);
    if(nbr_i >= 0) printf("Neighbor %d: %04X, ts %d\n", nbr_i, nbr.id, nbr.last_heard);
  }
}

static void rall(rall_descr_t* rall) {
  int i;

  printf("App: rall cb for init %X\n", rall->init);
  for (i = 0; i < rall->num_nodes; i++) {
    printf("\t%X:%d", rall->nodes[i], rall->distances[i]);
  }
  printf("\n");
}

static void adjm(adjm_descr_t* adjm) {
  int i, j;
  int matrix_location;

  printf("App: adjm cb\n");
  for (i = 0; i < adjm->num_nodes; i++) {
    printf("\t%X", adjm->nodes[i]);
    for (j = 0; j < adjm->num_nodes; j++) {
      matrix_location = i * adjm->num_nodes + j;
      printf("\t%X:%d", adjm->nodes[j], (adjm->matrix)[matrix_location].distance);
    }
    printf("\n");
  }

  printf("ADJM begin\n");

	usleep(50000);
  //////MARCO/////
	int status = send(0x0, vector_test, 7, 15);
	if(status != OQ_SUCCESS) {
		fprintf(stderr, "broadcast failed. Status: %s\n", (status == OQ_SUCCESS) ? "OQ_SUCCESS" : (status == OQ_FULL ? "OQ_FULL" : (status == OQ_FAIL ? "OQ_FAIL" : "OQ_UNKNOWN")));
	}else
	{
		fprintf(stderr, "broadcast success! Status:  %s\n", (status == OQ_SUCCESS) ? "OQ_SUCCESS" : (status == OQ_FULL ? "OQ_FULL" : (status == OQ_FAIL ? "OQ_FAIL" : "OQ_UNKNOWN")));
	}
    //

  go_on = true;
}

int main(int argc, char *argv[]) {

  if (argc<2) {
    printf("Specify the device file name\n");
    return 1;
  }

  if(!init(argv[1])) {
    printf(">>>>>> !!! Failed to Initialize !!! <<<<<<\n");
  }
  register_sent_cb(sent);
  register_recv_cb(recv);
  register_rng_cb(dist);
  register_nbr_cb(nbr);
  register_rall_cb(rall);
  register_adjm_cb(adjm);

  uint8_t a[4] = {'c', 'i', 'a', 'o'};
  node_id_t nodes[3] = {0x379e, 0x39d0, 0x3839};

  //power_off();
  //power_on();

  printf("My device id: %04X\n", get_node_id());

  printf("ADJM begin\n");

  go_on = true;
  sleep(5);

  while (1) {
    int nbr_i;
    /*nbr_t nbr;
    nbr_i = nbr_table_next(-1, &nbr);
    if(nbr_i >= 0) printf("Neighbor %d: %04X, ts %d\n", nbr_i, nbr.id, nbr.last_heard);
    while(nbr_i != -1) {
      nbr_i = nbr_table_next(nbr_i, &nbr);
      if(nbr_i >= 0) printf("Neighbor %d: %04X, ts %d\n", nbr_i, nbr.id, nbr.last_heard);
    }*/

    clock_gettime(CLOCK_MONOTONIC_RAW, &monitor_before);
    while(!build_adjm(NULL, 3)) {
      printf("ADJM failed, retrying...\n");
      sleep(1);
    }
    
    //break;
    //sleep(10);
    
    while (!go_on) { /*sleep(1);*/ }
    go_on = false;
    
    
    usleep(500000);
    ////////////////

  }
  while (1) { sleep(1); };
  return 0;
}
