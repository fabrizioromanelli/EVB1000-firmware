#include "cubes_api.h"
#include <stdio.h>
#include <unistd.h>

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
  printf("App: recv from %x, len %d\n", src, recv_len);
}

static void nbr(nbr_t node, bool status) {
  printf("App: nbr cb for node %X, %d\n", node.id, status);
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

  printf("My device id: %04X\n", get_node_id());

  while (1) {
    sleep(1);
    uint8_t a[] = {0,1,2,3,4,5};
    //send(0x114A, a, sizeof(a), 0);
    sleep(1);
    //range_with(0x114A);
  }
  return 0;
}
