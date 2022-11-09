#include "cubes_api.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>

#define MAX_HANDLE 1000
#define DESTINATION 0x0dbb

uint8_t a[] = {0,1,2,3,4,5};

struct timespec start, end;
uint16_t successful;
uint32_t successful_total;
uint32_t rounds_total;

static void sent(node_id_t dst, uint16_t handle, enum tx_status status) {
  //printf("App: sent %X, %d, %d\n", dst, handle, status);

  if(status == TX_SUCCESS) {
    successful++;
    successful_total++;
  }

  if(handle == MAX_HANDLE - 1) {
    printf("App: sent %X, %d, %d\n", dst, handle, status);

		clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    uint64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
    printf("Elapsed time (%d tx): %llu us\n", MAX_HANDLE, delta_us);
    printf("Successful: %d/%d, %d/%d\n", successful, MAX_HANDLE, successful_total, MAX_HANDLE * rounds_total);

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    handle = 0;
    successful = 0;
    rounds_total++;

    send(DESTINATION, a, sizeof(a), 0);
  }
  else {
    send(DESTINATION, a, sizeof(a), ++handle);
  }
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

  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  successful = 0;
  successful_total = 0;
  rounds_total = 1;

  send(DESTINATION, a, sizeof(a), 0);

  while(1) {
    sleep(1);
  }

  return 0;
}
