#include "cubes_api.h"
#include <stdio.h>
#include <unistd.h>

uint8_t a[] = {0,1,2,3,4,5};

node_id_t node_ping = 0x170B; //0x114A;
node_id_t node_pong = 0x3839; //0x02D8;
node_id_t node_this = 0;

int got_reply = 0;

static void sent(node_id_t dst, uint16_t handle, enum tx_status status) {
  printf("App: sent %X, %d, %d\n", dst, handle, status);
}

static void dist(nbr_t node, int status) {
  printf("App: distance %X, %d, %d\n", node.id, status, node.distance);

  got_reply = 1;
}

static void recv() {
  node_id_t src;
  uint8_t buf[MAX_PAYLOAD_LEN];
  int recv_len = pop_recv_packet(buf, &src);
  printf("App: recv from %x, len %d\n", src, recv_len);

  got_reply = 1;
}

int main(int argc, char *argv[]) {
	uint16_t packet_handle = 0;

  if (argc<2) {
    printf("Specify the device file name\n");
    return 1;
  }

  init(argv[1]);
  register_sent_cb(sent);
  register_recv_cb(recv);
  register_rng_cb(dist);
  node_this = get_node_id();

  /* assign role based on node id */
  printf("Node id: %04X\n", node_this);
  if (node_this == node_ping) {
    printf("Node ping.\n");
    got_reply = 1;
  }
  else if (node_this == node_pong) {
    printf("Node pong.\n");
    got_reply = 0;
  }
  else {
    printf("Error in node addresses!\n");
    return 0;
  }

  while (1) {
    sleep(1);

    if(node_this == node_ping) {
      if(got_reply == 1) {
        got_reply = 0;
        send(node_pong, a, sizeof(a), packet_handle++);
        //range_with(node_pong);
      }
    }

    if(node_this == node_pong) {
      if(got_reply == 1) {
        got_reply = 0;
        send(node_ping, a, sizeof(a), packet_handle++);
        //range_with(node_ping);
      }
    }


  }
  return 0;
}
