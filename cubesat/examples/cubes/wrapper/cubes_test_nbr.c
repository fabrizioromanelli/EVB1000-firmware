#include "cubes_api.h"
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

struct timespec start, end;

static void sent(node_id_t dst, uint16_t handle, enum tx_status status) {
  printf("App: sent %X, %d, %d\n", dst, handle, status);
}

static void dist(nbr_t node, int status) {
  printf("App: distance %X, %d, %d\n", node.id, status, node.distance);
}

static void recv() {
  node_id_t src;
  uint8_t buf[MAX_PAYLOAD_LEN];
  int recv_len = pop_recv_packet(buf, &src);
  printf("App: recv from %04X, len %d\n", src, recv_len);
}

static void nbr(nbr_t node, bool status) {
  printf("App: nbr cb for node %04X, %d\n", node.id, status);
}

static void rall(rall_descr_t* rall) {
  int i;

  printf("App: rall cb for init %X\n", rall->init);
  for (i = 0; i < rall->num_nodes; i++) {
    printf("\t%04X:%d", rall->nodes[i], rall->distances[i]);
  }
  printf("\n");
}

static void adjm(adjm_descr_t* adjm) {
  int i, j;
  int matrix_location;

  printf("App: adjm cb (%d nodes)\n", adjm->num_nodes);
  for (i = 0; i < adjm->num_nodes; i++) {
    printf("\t%04X", adjm->nodes[i]);
    for (j = 0; j < adjm->num_nodes; j++) {
      matrix_location = i * adjm->num_nodes + j;
      printf("\t%04X:%d", adjm->nodes[j], (adjm->matrix)[matrix_location].distance);
    }
    printf("\n");
  }

  clock_gettime(CLOCK_MONOTONIC_RAW, &end);
  uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
  printf("Elapsed time: %llu us\n", delta_us);
}

int main(int argc, char *argv[]) {

  if (argc<2) {
    printf("Specify the device file name\n");
    return 1;
  }

  /*while(!init(argv[1])) {
    printf("Init failed, retrying in 1 second\n");
    sleep(1);
  }*/
  if(!init(argv[1])) {
    printf(">>>>>> !!! Failed to Initialize !!! <<<<<<\n");
  }
  register_sent_cb(sent);
  register_recv_cb(recv);
  register_rng_cb(dist);
  register_nbr_cb(nbr);
  register_rall_cb(rall);
  register_adjm_cb(adjm);

  uint8_t a[] = {'h', 'a', 'l', 'o'};
  //node_id_t nodes[3] = {0x379e, 0x39d0, 0x3839};

  printf("My device id: %04X\n", get_node_id());

  int nbr_i;
  nbr_t nbr;
  while (1) {
    nbr_i = 0;
    sleep(4);
    nbr_i = nbr_table_next(-1, &nbr);
    if(nbr_i >= 0) printf("Neighbor %d: %04X, ts %d\n", nbr_i, nbr.id, nbr.last_heard);
    while(nbr_i != -1) {
      nbr_i = nbr_table_next(nbr_i, &nbr);
      if(nbr_i >= 0) printf("Neighbor %d: %04X, ts %d\n", nbr_i, nbr.id, nbr.last_heard);
    }
    /*if(nbr_table_remove(1)) {
      printf("*** REMOVED NODE 1 ***\n");
      nbr_i = nbr_table_next(-1, &nbr);
      if(nbr_i >= 0) printf("Neighbor %d: %04X, ts %d\n", nbr_i, nbr.id, nbr.last_heard);
      while(nbr_i != -1) {
        nbr_i = nbr_table_next(nbr_i, &nbr);
        if(nbr_i >= 0) printf("Neighbor %d: %04X, ts %d\n", nbr_i, nbr.id, nbr.last_heard);
      }
    }*/
    sleep(2);
    printf("*** SEND ***\n");
    send(0x1832, a, sizeof(a), 77);
    sleep(2);
    printf("*** RALL ***\n");
    multi_range_with(0x1832, NULL, 3);
    sleep(2);

		clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    printf("*** ADJM ***\n");
    /*while(!build_adjm(NULL, 3)) {
      printf("ADJM failed, retrying...\n");
      sleep(1);
    }*/
    build_adjm(NULL, 3);
  }
  return 0;
}
