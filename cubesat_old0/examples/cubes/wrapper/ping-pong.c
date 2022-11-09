#include "cubes_api.h"
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

uint8_t a[] = {0,1,2,3,4,5};
uint16_t packet_handle = 0;

node_id_t node_ping = 0x4c06; //0x114A;
node_id_t node_pong = 0x0dbb; //0x02D8;
node_id_t node_this = 0x4c06;
node_id_t this=0, other=0;

static void sent(node_id_t dst, uint16_t handle, enum tx_status status) {
  printf("App: sent %X, %d, %d\n", dst, handle, status);
	if (status != TX_SUCCESS) printf("Error in tx.\n");
}

static void dist(nbr_t node, int status) {
  printf("App: distance %X, %d, %d\n", node.id, status, node.distance);
}

static void recv() {
  node_id_t src;
  uint8_t buf[MAX_PAYLOAD_LEN];
  int recv_len = pop_recv_packet(buf, &src);
  printf("App: recv from %x, len %d, buf %s\n", src, recv_len, buf);

  packet_handle++;
  //send(other, a, sizeof(a), packet_handle);

}

int main(int argc, char *argv[]) {
  if (argc<5) {
    printf("Specify the serial device file name, this node id and other node id and payload\n");
    return 1;
  }
  this = (node_id_t) strtol(argv[2],NULL,16);
  other = (node_id_t) strtol(argv[3],NULL,16);
  printf("Serial dev %s, this=%04X, other=%04X",argv[1],this, other );

  init(argv[1]);
  register_sent_cb(sent);
  register_recv_cb(recv);
  register_rng_cb(dist);
  node_this = get_node_id();

  send(other, argv[4], sizeof(a), packet_handle);


  while(1) {
    sleep(1000);
  }

  return 0;
}
