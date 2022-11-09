#include "cubes_api.h"
#include <stdio.h>
#include <unistd.h>

#define RALL_DESTINATION 0x379e

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

  printf("RALL begin\n");
  while(!multi_range_with(RALL_DESTINATION, NULL, /*0*/ 3))
    printf("RALL err\n");
}

int main(int argc, char *argv[]) {

  if (argc<2) {
    printf("Specify the device file name\n");
    return 1;
  }

  init(argv[1]);
  register_sent_cb(sent);
  register_recv_cb(recv);
  register_rng_cb(dist);
  register_nbr_cb(nbr);
  register_rall_cb(rall);

  node_id_t nodes[3] = {0x379e, 0x39d0, 0x3839};

  printf("My device id: %04X\n", get_node_id());

  sleep(4);

  printf("RALL begin\n");
  while(!multi_range_with(RALL_DESTINATION, NULL, /*0*/ 3))
    printf("RALL err\n");

  while (1) {
    sleep(1);
  }

  return 0;
}
