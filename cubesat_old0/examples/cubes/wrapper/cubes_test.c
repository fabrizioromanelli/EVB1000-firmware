#include "cubes_api.h"
#include <stdio.h>
#include <unistd.h>

#define DESTINATION 0xFFFF

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

int main(int argc, char *argv[]) {

  if (argc<2) {
    printf("Specify the device file name\n");
    return 1;
  }
    
  if (!init(argv[1]))
  {
    printf("Failed to initialise\n");
    return 0;
  }

  node_id_t my_id = get_node_id();
  printf("My ID: %04X\n", my_id);

  register_sent_cb(sent);
  register_recv_cb(recv);
  register_rng_cb(dist);

  while (1) {
    sleep(1);
    uint8_t a[MAX_PAYLOAD_LEN];
    if(send(DESTINATION, a, 16, 0) == OQ_FULL)
    	fprintf(stderr, "Out queue full\n");
    sleep(1);
    range_with(DESTINATION);
  }
  return 0;
}

