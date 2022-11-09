#include "cubes_api.h"
#include <stdio.h>
#include <unistd.h>

static void sent(node_id_t dst, uint16_t handle, enum tx_status status) {
  printf("App: sent %X, %d, %d\n", dst, handle, status);
}

static void dist(nbr_t node, int status) {
  printf("App: distance %X, %d, %d, ts %u\n", node.id, status, node.distance, node.last_rng_time);
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

  printf("My device id: %04X\n", get_node_id());

  while (1) {
    uint8_t a[] = {0,1,2,3,4,5};
    //send(0x114A, a, sizeof(a), 0);
    sleep(5);
    if(!range_with(0x379e)) printf("RNG failed.\n");
  }
  return 0;
}
