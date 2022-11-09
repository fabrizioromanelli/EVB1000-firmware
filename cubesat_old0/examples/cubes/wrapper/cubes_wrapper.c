#include "cubes_api.h"
#include "cubes_common.h"
#include "serial.h"
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <pthread.h>

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

int serial_fd;
pthread_t serial_tid;

static enum {
  ST_OFF,
  ST_IDLE,
  ST_SENDING,
  ST_RANGING
} state;

#define IS(cmd) (strncmp((char*)input, #cmd, 4) == 0)
#define IS_OK()   (strncmp((char*)input+5, "_OK_", 4) == 0)
#define IS_NOA()  (strncmp((char*)input+5, "NO_A", 4) == 0)
#define IS_FAIL() (strncmp((char*)input+5, "FAIL", 4) == 0)
#define IS_BUSY() (strncmp((char*)input+5, "BUSY", 4) == 0)
#define IS_OFF()  (strncmp((char*)input+5, "OFF_", 4) == 0)

// Request data
uint16_t send_handle;
node_id_t send_dst;
node_id_t ranging_with;

// XXX temporary stuff before we have a queue implemented
node_id_t recv_src_id;
uint16_t recv_len;
uint8_t recv_buf[MAX_MODULE_PACKET_LEN];
uint8_t recv_qlen;
pthread_mutex_t recv_mutex = PTHREAD_MUTEX_INITIALIZER;

// callbacks
recv_cb_t recv_callback;
sent_cb_t sent_callback;
rng_cb_t  rng_callback;

// device information
node_id_t device_addr;

int decode_recv(const char* input, uint16_t in_len) {
  node_id_t src_id;
  uint16_t len;

  if (in_len < 14 || input[4] != ' ' || input[9] != ' ')
    return 0;
  if (!decode_hex(input+5, 4, (char*)&src_id))
    return 0;
  if (!decode_hex(input+10, 4, (char*)&len))
    return 0;

  len = ntohs(len);
  src_id = ntohs(src_id);

  if (len == 0) {
    recv_len = len;
    recv_src_id = src_id;
    return 1;
  }
  if (len > MAX_MODULE_PACKET_LEN)
    return 0;
  if ((in_len-15)/2 != len)
    return 0;

  char tmp[len];
  if (decode_hex(input+15, in_len-15, tmp)) {
    recv_len = len;
    recv_src_id = src_id;
    memcpy(recv_buf, tmp, len);
    return 1;
  }

  return 0;
}

void process_packet(const char *input, uint16_t len) {
  if (IS(RECV)) {
    int received;
    pthread_mutex_lock(&recv_mutex);
    received = decode_recv(input, len);

    // XXX we need to distinguish our control packets from App data packets
    received = received && (recv_len <= MAX_PAYLOAD_LEN);

    if (received) {
      recv_qlen = 1;
    }
    pthread_mutex_unlock(&recv_mutex);
    if (received && recv_callback)
      recv_callback();
  }
  else if (IS(SEND)) {
  }
  else if (IS(SENT)) {
    if (state == ST_SENDING) {
      state = ST_IDLE; // XXX use mutex?
      if (sent_callback) {
        if (IS_OK())
          sent_callback(send_dst, send_handle, TX_SUCCESS);
        else if (IS_NOA())
          sent_callback(send_dst, send_handle, TX_NOACK);
        else
          sent_callback(send_dst, send_handle, TX_FAIL);
      }
    }
  }
  else if (IS(DIST)) {
    if (state == ST_RANGING) {
      state = ST_IDLE;
      if (rng_callback) {
        nbr_t nbr;
        node_id_t node;
        nbr.id = 0;
        nbr.distance = 0;
        if (decode_hex(input+10, 4, (char*)&node)) {
          nbr.id = ntohs(node);
          if (IS_OK()) {
            double distance;
            if (sscanf(input+15, "%lf", &distance)) {
              if (distance < 0) distance = 0;
              nbr.distance = distance * 100; // meters to cm
              rng_callback(nbr, 1);
            }
            else {/*decode error: should not happen*/
              rng_callback(nbr, 0);
            }
          }
          else {
            rng_callback(nbr, 0);
          }
        }
        else {/*decode error: should not happen*/}
      }
    }
  }
  else if (IS(RNGE)) {
  }
  else if (IS(REST)) {
  }
}

#define SERIAL_BUF_LEN 2048

void* serial_reader(void* p) {
  char buf[SERIAL_BUF_LEN+1];
  int idx;
  int truncated;
  do {
    char c;
    int rdlen;
    idx = 0;
    truncated = 0;
    do {
      rdlen = read(serial_fd, &c, 1);
      if (rdlen > 0) {
        if (idx < SERIAL_BUF_LEN)
          buf[idx++] = c;
        else
          truncated = 1;
        if (c == '\n')
          break; // complete packet
      } else if (rdlen == 0) {
        printf("Zero bytes read\n");
      }
      else {
        printf("Error from read: %d: %s\n", rdlen, strerror(errno));
        sleep(1); // XXX might have lost the serial connection. Try to reopen? Inform the app?
      }
    } while (1);
    buf[idx] = '\0';
    if (truncated) {
      printf("Truncated serial packet: %s\n", buf); // drop truncated packets
    }
    else {
      PRINTF("From serial: %s", buf);
      process_packet(buf, idx-1 /* to cut the '\n' */);
    }
  } while (1);
}

static int send_serial(char* packet, int len) {
  int wlen = write(serial_fd, packet, len);
  if (wlen != len) {
      printf("Error from write: %d, %d\n", wlen, errno);
      return 0;
  }
  tcdrain(serial_fd);    /* delay for output */
  return 1;
}

int init(const char *serial_device) {

  /* open serial port */
  serial_fd = serial_open(serial_device);
  if (serial_fd < 0) {
    printf("Error opening %s: %s\n", serial_device, strerror(errno));
    return 0;
  }
  printf("Port is open: %s\n", serial_device);

  /* serial configuration */
  if (!serial_set_interface_attribs(serial_fd, B115200)) {
    printf("Error setting port attributes\n");
    return 0;
  }

  /* flush serial data (both directions) */
  usleep(10000); // required for USB serial flush
  tcflush(serial_fd,TCIOFLUSH);

  /* prepare serial configuration for the address request */
  struct termios request_attr;
  struct termios default_attr;
  tcgetattr(serial_fd, &default_attr);
  request_attr = default_attr;
  request_attr.c_cc[VMIN] = 10; // will stop reading when 10 bytes are received
  request_attr.c_cc[VTIME] = 10; // or after a while (tenths of sec) without transmissions
  if (tcsetattr(serial_fd, TCSANOW, &request_attr) != 0) {
    printf("Error from tcsetattr: %s\n", strerror(errno));
    return 0;
  }

  /* request the device address */
  char request[6];
  char input[11];
  snprintf(request, 5, "ADDR");
  request[4] = '\n';
  request[5] = '\0';
  device_addr = 0;
  if (!send_serial(request, 5)) {
    return 0;
  }
  read(serial_fd, input, sizeof(input));

  /* save the address locally */
  if (IS(ADDR)) {
    node_id_t addr;
    if (decode_hex(input+5, 4, (char*)&addr)) {
      device_addr = ntohs(addr);
    }
    else {
      printf("Error in decoding module's reply.\n");
      return 0;
    }
  }
  else {
    printf("Error in serial reading.\n");
    return 0;
  }

  /* restore default serial configuration */
  if (tcsetattr(serial_fd, TCSANOW, &default_attr) != 0) {
    printf("Error from tcsetattr: %s\n", strerror(errno));
    return 0;
  }

  /* create the dedicated thread for the serial input */
  if (pthread_create(&serial_tid, NULL, serial_reader, NULL)) {
    printf("Error starting a thread\n");
    return 0;
  }

  state = ST_IDLE;
  return 1;
}

enum out_queue_status send(node_id_t dst, const uint8_t* buf, uint8_t length, uint16_t handle) {
  if (state != ST_IDLE)
    return OQ_FULL;

  int encoded_length = 15 + length*2 + 1; // +1 is for \n
  char packet[encoded_length + 1];        // +1 is for \0

  snprintf(packet, 16, "SEND %04X %04X ", dst, length);
  encode_hex((const char*)buf, length, packet+15);
  packet[encoded_length-1] = '\n';
  packet[encoded_length] = '\0';
  //printf("%s", packet);
  if (!send_serial(packet, encoded_length)) {
    return OQ_FAIL;
  }
  state = ST_SENDING;
  send_handle = handle;
  send_dst = dst;

  return OQ_SUCCESS;
}


int range_with(node_id_t node) {
  if (state != ST_IDLE)
    return 0;

  int encoded_length = 9 + 1;       // +1 is for \n
  char packet[encoded_length + 1];  // +1 is for \0

  snprintf(packet, encoded_length+1, "RNGE %04X\n", node);
  if (!send_serial(packet, encoded_length)) {
    return 0;
  }
  state = ST_RANGING;

  return 1;
}

int pop_recv_packet(uint8_t* buf, node_id_t* from) {
  int res = 0;
  pthread_mutex_lock(&recv_mutex);
  if (recv_qlen > 0) {
    memcpy(buf, recv_buf, recv_len);
    recv_qlen = 0;
    *from = recv_src_id;
    res = recv_len;
  }
  pthread_mutex_unlock(&recv_mutex);
  return res;
}

void register_sent_cb(sent_cb_t cb) {
  sent_callback = cb;
}

void register_recv_cb(recv_cb_t cb) {
  recv_callback = cb;
}

void register_rng_cb(rng_cb_t cb) {
  rng_callback = cb;
}

node_id_t get_node_id(void) {
  return device_addr;
}
