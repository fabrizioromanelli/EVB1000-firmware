#include "contiki.h"
#include "lib/random.h"
#include "leds.h"
#include "net/netstack.h"
#include "net/rime/rime.h"
#include <stdio.h>
#include <math.h>
#include "dw1000.h"
#include "dw1000-ranging.h"
#include "core/net/linkaddr.h"
#include "serial-line.h"
#include "net/packetbuf.h"

#include "packetman.h"
#include "cubes_common.h"

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif

/* Serial buffer */
#define MAX_RAW_BUF_LEN 128
#define MAX_ENCODED_BUF_LEN MAX_RAW_BUF_LEN*2

/* Rime channels */
#define CUBES_RIME_CHANNEL 150
#define CUBES_BEACON_CHANNEL 129 //0x81
#define CUBES_BEACON_RESPONSE_CHANNEL 130 //0x82

/* Beacons for neighbor discovery */
#define BEACON_PERIOD (CLOCK_SECOND * 5)
#define BEACON_MAX_RESPONSE_DELAY (CLOCK_SECOND / 4)
#define BEACON_SHOULD_RESPOND 0

/* Ranging */
#define RNG_MAX_ATTEMPTS 3 // max number of attempts in a ranging procedure
#define RALL_REPORT_WAIT (CLOCK_SECOND / 2) // time to consider a remote ranging request as failed
#define ADJM_COMPLETE_WAIT (CLOCK_SECOND / 2) // time to consider an ADJM message lost

/*
 * - Serial protocol specification ------------------------------------
 * Everything apart from the distance is in HEX and big-endinan.
 * Distance is a decimal fraction in meters (XXX maybe change this).
 *
 * - Requests ---------------------------------------------------------
 * SEND dest len_ data
 * RNGE dest
 * RALL init nodelist
 * POFF
 * PON_
 * ADDR
 * AMBC
 *
 * - Status response --------------------------------------------------
 * [SEND, RNGE, RALL] [_OK_, OFF_]
 *
 * Note that only the _OK_ response for SEND, RNGE, RALL requests means that
 * there will be an indication corresponding to the current request.
 *
 * - Indications ------------------------------------------------------
 * SENT [_OK_, FAIL, NO_A]
 * RECV src_ len_ data
 * DIST [_OK_ init distance, FAIL init]
 * DALL init node-distance-list
 * CERR 'original_line'
 */

 #define IS(cmd) (strncmp((char*)input, #cmd, 4) == 0)

 #define _EXTR_CMD() ((uint8_t*)data)[0], ((uint8_t*)data)[1], ((uint8_t*)data)[2], ((uint8_t*)data)[3]
 #define _PRINT_ERR(resp) printf("%c%c%c%c " #resp "\n", _EXTR_CMD())
 #define OK() _PRINT_ERR(_OK_)
 #define CERR() printf("CERR '%s'\n", (char*)data)

/* State of the module */
static enum state {
  ST_OFF,
  ST_IDLE,
  ST_SENDING,
  ST_RNGE,
  ST_RALL,
  ST_RALL_REMOTE_REQ,
  ST_RALL_REMOTE_INIT,
  ST_ADJM_RECV
} state;

/* Message type to distinguish app data, ranging coordination and report */
typedef enum message_type {
  MSG_APP, // app data
  MSG_RALL_CONTROL, // initiator's RALL request to another node
  MSG_RALL_REPORT, // report RALL results to the initiator
  MSG_ADJM // broadcast of complete adjacency matrix
} message_type;

/* Packet header (contains message type) */
typedef struct {
  message_type type;
} __attribute__((packed))
message_header;

/*---------------------------------------------------------------------------*/

static void beacon_recv_bc(struct broadcast_conn *c, const linkaddr_t *from);
static void beacon_recv_uc(struct unicast_conn *c, const linkaddr_t *from);
static void beacon_sent_uc(struct unicast_conn *c, int status, int num_tx);
int decode_addr(const char* input, uint16_t in_len);
int decode_range(const char* input, uint16_t in_len, uint16_t* dst);
int decode_range_all(const char* input, uint16_t in_len, uint16_t* init, uint16_t* dst_list, uint16_t* num_dst);
int decode_ambc(const char* input, uint16_t in_len, uint16_t* init, uint16_t* dst_list, uint16_t* num_dst,
uint16_t* adjm_distances_list, uint8_t* current_adjm_msg, uint8_t* total_adjm_msg);
int decode_send(const char* input, uint16_t in_len, uint16_t* dst, uint16_t* datalen, char* buf);
void discovery_beacon();
void discovery_init(uint8_t _should_respond);
static void recv(struct broadcast_conn *c, const linkaddr_t* from);
static void reply_cb(void* ptr);
static void sent(struct broadcast_conn *c, int status, int num_tx);

/*---------------------------------------------------------------------------*/

/* Serial communication between module and wrapper */
char buffer[MAX_ENCODED_BUF_LEN + 1]; // +1 for zero-termination
static int input_len;

/* Standard transmission and reception */
const linkaddr_t linkaddr_broadcast = {{ 0x00, 0x00 }};
static struct broadcast_conn bc; // using broadcast because unicast of Rime doesn't support broadcast
static const struct broadcast_callbacks broadcast_callbacks = {recv, sent};
static const struct packetbuf_attrlist attributes[] = {
  {PACKETBUF_ADDR_RECEIVER, PACKETBUF_ADDRSIZE},
  BROADCAST_ATTRIBUTES
  PACKETBUF_ATTR_LAST
};

/* Beacon management (for neighbor discovery) */
static struct unicast_conn beacon_uc;
static struct broadcast_conn beacon_bc;
static const struct broadcast_callbacks beacon_broadcast_callbacks = {beacon_recv_bc};
static const struct unicast_callbacks beacon_unicast_callbacks = {beacon_recv_uc, beacon_sent_uc};
static struct ctimer reply_timer; // for random reply delays
static uint8_t replying;  // a flag indicating that we are currently replying
static uint8_t should_respond; // whether we should respond to beacons
static linkaddr_t reply_to;  // the node we should reply to
static uint8_t discovery_active; // if enabled, the device periodically transmits a beacon

/* Ranging procedures */
static uint16_t dst;
static uint16_t dst_list[MAX_NODES + 1];
static double distances_list[MAX_NODES + 1];
static uint16_t num_dst;
static uint16_t datalen;
static linkaddr_t current_peer; // the remote node we are currently working with as dst
static uint16_t rall_init; // address of the ranging-with-all initiator in u16 format
static linkaddr_t current_init;
static linkaddr_t rall_requester;

/* Adjencency matrix acquisition */
static uint16_t adjm_distances_list[MAX_NODES + 1];
static uint8_t current_adjm_msg;
static uint8_t total_adjm_msg;
static linkaddr_t current_adjm_init;

/* Timer to recover from a remote RALL whose reply was not received */
static struct ctimer rall_remote_init_timer;

/* Timer to recover when waiting for ADJM messages */
static struct ctimer adjm_complete_timer;

/*---------------------------------------------------------------------------*/

PROCESS(cubes_main, "Cubes module main process");
PROCESS(cubes_ranging, "Single node ranging process");
PROCESS(cubes_ranging_all, "Multiple nodes ranging process");
PROCESS(cubes_discovery, "Neighbors discovery process");
AUTOSTART_PROCESSES(&cubes_main);

/*---------------------------------------------------------------------------*/

int decode_send(const char* input, uint16_t in_len, uint16_t* dst, uint16_t* datalen, char* buf) {
  if (in_len < 14 || !IS(SEND) || input[4] != ' ' || input[9] != ' ')
    return 0;
  if (!decode_hex(input+5, 4, (char*)dst))
    return 0;
  if (!decode_hex(input+10, 4, (char*)datalen))
    return 0;

  *datalen = ntohs(*datalen);
  if (*datalen == 0)
    return 1;
  if (*datalen > MAX_RAW_BUF_LEN)
    return 0;
  if ((in_len-15)/2 != *datalen)
    return 0;

  return decode_hex(input+15, in_len-15, buf);
}

int decode_range(const char* input, uint16_t in_len, uint16_t* dst) {
  if (in_len != 9 || !IS(RNGE) || input[4] != ' ')
    return 0;

  return decode_hex(input+5, 4, (char*)dst);
}

int decode_range_all(const char* input, uint16_t in_len, uint16_t* init, uint16_t* dst, uint16_t* num_dst) {
  PRINTF("dbg: decode RALL ('%s')\n", input);

  if (in_len < 11 || !IS(RALL) || input[4] != ' ')
    return 0;

  PRINTF("dbg: decode ok (1)\n");

  if (!decode_hex(input+5, 4, (char*)init))
    return 0;

  PRINTF("dbg: Init %04X\n", *init);

  int i = 0;
  *num_dst = 0;
  while(input[4+((i+1)*5)] == ' ') {
    PRINTF("dbg: Reading %c%c%c%c\n",
      *(input+5+((i+1)*5)+0),
      *(input+5+((i+1)*5)+1),
      *(input+5+((i+1)*5)+2),
      *(input+5+((i+1)*5)+3));
    if (!decode_hex(input+5+((i+1)*5), 4, (char*)&dst[i]))
      return 0;
    PRINTF("dbg: Acquired destination %04X\n", dst[i]);
    (*num_dst)++;
    i++;
  }

#if DEBUG
  for (i = 0; i < *num_dst; i++) {
    PRINTF("dbg: Destination %d is %04X\n", i, dst[i]);
  }
#endif

  if (*num_dst < 1)
    return 0;

  return 1;
}

int decode_ambc(const char* input, uint16_t in_len, uint16_t* init, uint16_t* dst_list, uint16_t* num_dst,
uint16_t* adjm_distances_list, uint8_t* current_adjm_msg, uint8_t* total_adjm_msg) {
  int in_read = 5;
  int i;
  int num_bytes;

  PRINTF("dbg: decode AMBC ('%s')\n", input);

  if (in_len < 14 || !IS(AMBC) || input[4] != ' ')
    return 0;

  PRINTF("dbg: decode ok (1)\n");

  /* retrieve message id and the number of nodes */
  if (sscanf(input+in_read, "%hhu%n", current_adjm_msg, &num_bytes)) {
    PRINTF("dbg: current_adjm_msg %hhu\n", *current_adjm_msg);

    in_read += num_bytes + 1; // +1 for the following space
  }
  else {
    PRINTF("dbg: decode failed (2)\n");

    return 0;
  }
  if (sscanf(input+in_read, "%hhu%n", total_adjm_msg, &num_bytes)) {
    PRINTF("dbg: total_adjm_msg %hhu\n", *total_adjm_msg);

    in_read += num_bytes + 1; // +1 for the following space
  }
  else {
    PRINTF("dbg: decode failed (3)\n");

    return 0;
  }

  /* extract initiator */
  if (!decode_hex(input+in_read, 4, (char*)init))
    return 0;
  in_read += 5;

  PRINTF("dbg: Init %04X\n", *init);

  /* as long as there is something to decode */
  i = 0;
  while (in_len - in_read >= 4) {
    PRINTF("Decode state: in_read %d, result_num %d, num_nodes %u", in_read, i, *total_adjm_msg);
    if (decode_hex(input+in_read, 4, (char*)&dst_list[i])) {
      PRINTF("Decoded node %04X\n", dst_list[i]);
      in_read += 5;
      if (sscanf(input+in_read, "%hu%n", &adjm_distances_list[i], &num_bytes)) {
        PRINTF("Decoded distance %u (%d bytes)\n", adjm_distances_list[i], num_bytes);
        in_read += num_bytes + 1; // %n in the scan saves the number of bytes read so far, +1 for space
        i++;
      }
      else break;
    }
    else break;
  }
  *num_dst = i;

#if DEBUG
  for (i = 0; i < *num_dst; i++) {
    PRINTF("dbg: Destination %d is %04X\n", i, dst_list[i]);
  }
#endif

  if (*num_dst < 1)
    return 0;

  return 1;
}

int decode_addr(const char* input, uint16_t in_len) {
  if (in_len != 4 || !IS(ADDR))
    return 0;

  return 1;
}

int decode_poff(const char* input, uint16_t in_len) {
  if (in_len != 4 || !IS(POFF))
    return 0;

  return 1;
}

int decode_pon(const char* input, uint16_t in_len) {
  if (in_len != 4 || !IS(PON_))
    return 0;

  return 1;
}

static void adjm_complete_to_cb(void* ptr) {
  state = ST_IDLE;
  printf("ADJM 0 0\n");
}

static void
recv(struct broadcast_conn *c, const linkaddr_t* from) {

  const linkaddr_t* to = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);

  if (linkaddr_cmp(to, &linkaddr_node_addr) || to->u16 == 0 || to->u16 == 0xFFFF) {
    static message_header header;
    uint16_t raw_len = packetbuf_datalen();

    if (raw_len > MAX_RAW_BUF_LEN || raw_len < sizeof(header))
      return;

    /* retrieve header, containing the message type */
    if (!packetman_extract(&header, sizeof(header)))
      return;

    raw_len -= sizeof(header);

    /* if the message contains app data, encode and transmit to the wrapper */
    if (header.type == MSG_APP) {
      uint16_t enc_len = encode_hex((char*)packetbuf_dataptr(), raw_len, buffer);
      buffer[enc_len] = 0; // for printf
      printf("RECV %02X%02X %04X %s\n",
          from->u8[0], from->u8[1],
          raw_len,
          buffer);
    }
    /* request from another node to start a RALL procedure */
    else if (header.type == MSG_RALL_CONTROL) {
      PRINTF("dbg: Received remote RALL request\n");

      if (state == ST_IDLE) {
        PRINTF("dbg: Prepare for remote RALL\n");

        current_init.u16 = linkaddr_node_addr.u16;
        linkaddr_copy(&rall_requester, from);

        int i;
        packetman_extract(&num_dst, sizeof(num_dst));
        for (i = 0; i < num_dst; i++) {
          packetman_extract(&dst_list[i], sizeof(*dst_list));
        }

        state = ST_RALL_REMOTE_INIT;

        process_poll(&cubes_ranging_all);
      }
#if DEBUG
      else {
        PRINTF("dbg: Err, state %d\n", state);
      }
#endif
    }
    else if (header.type == MSG_RALL_REPORT) {
      PRINTF("dbg: Received remote RALL reply\n");

      char init_encoded[LINKADDR_SIZE * 2 + 1];
      char addr_encoded[LINKADDR_SIZE * 2 + 1];
      int i;
      uint16_t enc_len;

      if (state != ST_RALL_REMOTE_REQ) {
        PRINTF("dbg: Ignoring RALL_REPORT packet (module state is %d)\n", state);

        return;
      }

      /* since a report was received, prevent the request timeout */
      ctimer_stop(&rall_remote_init_timer);

      /* retrieve the nodes and their distance from this node */
      packetman_extract(&num_dst, sizeof(num_dst));
      for (i = 0; i < num_dst; i++) {
        packetman_extract(&dst_list[i], sizeof(*dst_list));
        packetman_extract(&distances_list[i], sizeof(*distances_list));
      }

      PRINTF("dbg: Got %d ranging measures\n", num_dst);

      /* encode results for serial communication to the wrapper */
      *buffer = '\0';
      enc_len = encode_hex((char*)&rall_init, LINKADDR_SIZE, init_encoded);
      init_encoded[enc_len] = '\0';
      for (i = 0; i < num_dst; i++) {
        enc_len = encode_hex((char*)&dst_list[i], LINKADDR_SIZE, addr_encoded);
        addr_encoded[enc_len] = '\0';
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer) - 1, " %s %.03f", addr_encoded, distances_list[i]);
      }
      state = ST_IDLE;
      num_dst = 0;
      printf("DALL %s%s\n", init_encoded, buffer);
    }
    else if (header.type == MSG_ADJM) {
      PRINTF("dbg: Received ADJM packet\n");

      char init_encoded[LINKADDR_SIZE * 2 + 1];
      char addr_encoded[LINKADDR_SIZE * 2 + 1];
      int i;
      uint16_t enc_len;

      if (state != ST_IDLE && state != ST_ADJM_RECV) {
        PRINTF("dbg: Ignoring ADJM packet (module state is %d)\n", state);

        return;
      }

      /* read the ADJM information */
      packetman_extract(&rall_init, sizeof(rall_init));
      packetman_extract(&current_adjm_msg, sizeof(current_adjm_msg));
      packetman_extract(&total_adjm_msg, sizeof(total_adjm_msg));
      packetman_extract(&num_dst, sizeof(num_dst));
      for (i = 0; i < num_dst; i++) {
        packetman_extract(&dst_list[i], sizeof(*dst_list));
        packetman_extract(&adjm_distances_list[i], sizeof(*adjm_distances_list));

        PRINTF("dbg: Extracted node %04X (%d / %d)\n", dst_list[i], i, num_dst);
      }

      PRINTF("dbg: From ADJM (%hhu out of %hhu), got %d ranging measures\n", current_adjm_msg, total_adjm_msg, num_dst);

      /* encode results for serial communication to the wrapper */
      *buffer = '\0';
      enc_len = encode_hex((char*)&rall_init, LINKADDR_SIZE, init_encoded);
      init_encoded[enc_len] = '\0';
      for (i = 0; i < num_dst; i++) {
        enc_len = encode_hex((char*)&dst_list[i], LINKADDR_SIZE, addr_encoded);
        addr_encoded[enc_len] = '\0';
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer) - 1, " %s %u", addr_encoded, adjm_distances_list[i]);
      }

      /* if we expect other messages, change state to wait for another MSG_ADJM */
      if (current_adjm_msg < total_adjm_msg) {
        if (state == ST_IDLE) {

          /* remember the current adjm broadcaster */
          linkaddr_copy(&current_adjm_init, from);

          /* if another MSG_ADJM is not received in time, inform the wrapper */
          ctimer_set(&adjm_complete_timer, ADJM_COMPLETE_WAIT, adjm_complete_to_cb, NULL);

          state = ST_ADJM_RECV;
        }
        else {

          /* if we receive from an unexpected initiator, conclude procedure */
          if (!linkaddr_cmp(&current_adjm_init, from)) {
            PRINTF("dbg: unexpected initiator\n");

            state = ST_IDLE;
            printf("ADJM 0 0\n");

            return;
          }

          ctimer_restart(&adjm_complete_timer);
        }
      }
      else {
        PRINTF("dbg: got last vector of matrix\n");

        if (state == ST_ADJM_RECV) {
          ctimer_stop(&adjm_complete_timer);
          state = ST_IDLE;
        }
      }
      num_dst = 0;

      /* inform wrapper */
      printf("ADJM %hhu %hhu %s%s\n", current_adjm_msg, total_adjm_msg, init_encoded, buffer);
    }
  }
}

static void
sent(struct broadcast_conn *c, int status, int num_tx) {
  int old_state = state;

  if (state != ST_SENDING && state != ST_RALL_REMOTE_REQ)
    return;

  // TODO: report also FAIL for unspecified error?
  if (old_state == ST_SENDING) {
    state = ST_IDLE;
    if (status == MAC_TX_OK)
      printf("SENT _OK_\n");
    else
      printf("SENT NO_A\n");
  }
  else if (old_state == ST_RALL_REMOTE_REQ) {
    if (status != MAC_TX_OK) {
      state = ST_IDLE; // note that the state becomes IDLE only if there was a Tx error
      printf("DALL %02X%02X\n", current_init.u8[0], current_init.u8[1]);
    }
  }
  //PRINTF("%d\n", num_tx);
}

static void rall_remote_init_to_cb(void* ptr) {
  state = ST_IDLE;
  printf("DALL %02X%02X\n", current_init.u8[0], current_init.u8[1]);
}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(cubes_main, ev, data)
{
  PROCESS_BEGIN();
  discovery_init(BEACON_SHOULD_RESPOND);
  process_start(&cubes_ranging, NULL);
  process_start(&cubes_ranging_all, NULL);
  broadcast_open(&bc, CUBES_RIME_CHANNEL, &broadcast_callbacks);
  channel_set_attributes(CUBES_RIME_CHANNEL, attributes);

  state = ST_IDLE;

  while(1) {
    PROCESS_WAIT_EVENT();
    if (ev == serial_line_event_message) {
      input_len = strlen(data);

      if (input_len < 4) {
        PRINTF("dbg: malformed input\n");

        CERR();
      }
      else if (decode_poff(data, input_len)) {
        if (state == ST_OFF) {
          PRINTF("dbg: already off\n");
          continue;
        }

        /* stop ranging processes */
        if (state == ST_RNGE) {
          process_exit(&cubes_ranging);
        }
        if (state == ST_RALL || state == ST_RALL_REMOTE_INIT) {
          process_exit(&cubes_ranging_all);
        }

        /* clear any data for ongoing operations */
        dst = 0;
        memset(dst_list, 0, (MAX_NODES + 1) * sizeof(*dst_list));
        memset(distances_list, 0, (MAX_NODES + 1) * sizeof(*distances_list));
        memset(adjm_distances_list, 0, (MAX_NODES + 1) * sizeof(*adjm_distances_list));
        num_dst = 0;
        datalen = 0;
        linkaddr_copy(&current_peer, &linkaddr_null);
        rall_init = 0;
        linkaddr_copy(&current_init, &linkaddr_null);
        linkaddr_copy(&rall_requester, &linkaddr_null);
        discovery_active = 0;

        /* turn to the OFF state to ignore any request */
        state = ST_OFF;

        /* shut down radio */
        dw1000_driver.off();

        PRINTF("dbg: turned off\n");

        //OK();
      }
      else if (decode_pon(data, input_len)) {
        if (state != ST_OFF) {
          PRINTF("dbg: already on\n");
          continue;
        }

        process_start(&cubes_ranging, NULL);
        process_start(&cubes_ranging_all, NULL);
        dw1000_driver.on();
        discovery_active = 1;
        state = ST_IDLE;

        PRINTF("dbg: turned on\n");

        //OK();
      }
      else if (decode_addr(data, input_len)) {
        printf("ADDR %02X%02X\n", linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
      }
      else if (decode_send((char*)data, input_len, &dst, &datalen, buffer)) {
        PRINTF("dbg: send to %x len %d\n", dst, datalen);

        if (state == ST_IDLE) {

          if (dst == 0xFFFF) // broadcast addr in Rime is 0
            dst = 0;

          /* prepare the packet (received from the wrapper) that was decoded */
          packetbuf_clear();
          packetbuf_copyfrom(buffer, datalen);
          current_peer.u16 = dst;
          packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &current_peer);

          /* add header with message type MSG_APP */
          static message_header header;
          if (datalen > 0) {
            header.type = MSG_APP;
            if(!packetman_sethdr(&header, sizeof(header))) {
              CERR(); // should never happen
              continue;
            }
          }

          /* transmit */
          if (broadcast_send(&bc)) {
            state = ST_SENDING;
            OK();
          }
          else {
            printf("SENT FAIL\n");
          }
        }
        else {
          printf("SENT FAIL\n");
        }
      }
      else if (decode_range(data, input_len, &dst)) {
        PRINTF("dbg: range with %X\n", dst);

        if (state == ST_IDLE) {
          if (dst == 0xFFFF || dst == 0) {
            CERR();
          }
          else {
            state = ST_RNGE;
            current_peer.u16 = dst;
            OK();
            process_poll(&cubes_ranging);
          }
        }
        else {
          printf("DIST FAIL %02X%02X\n",
            current_peer.u8[0], current_peer.u8[1]);
        }
      }
      else if (decode_range_all(data, input_len, &rall_init, dst_list, &num_dst)) {
        PRINTF("dbg: Received RALL\n");

        if (state == ST_IDLE) {
          current_init.u16 = rall_init;

          PRINTF("dbg: node_addr %04X, init %04X\n", linkaddr_node_addr.u16, current_init.u16);

          /* if this node is the initiator for the RALL, range with the first node */
          if (linkaddr_cmp(&linkaddr_node_addr, &current_init)) {
            PRINTF("dbg: Polling RALL process\n");

            state = ST_RALL;
            OK();
            process_poll(&cubes_ranging_all);
          }
          /* otherwise, inform the actual initiator */
          else {
            PRINTF("dbg: Remote RALL request for %04X\n", current_init.u16);

            /* copy the node ids in the MSG_RALL_CONTROL packet */
            packetbuf_clear();
            packetman_setpayload(&num_dst, sizeof(num_dst));
            packetman_setpayload(dst_list, num_dst * sizeof(*dst_list));
            packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &current_init);

            /* add header with message type MSG_RALL_CONTROL */
            static message_header header;
            header.type = MSG_RALL_CONTROL;
            if(!packetman_sethdr(&header, sizeof(header))) {
              CERR();
              continue;
            }

            /* transmit */
            if (broadcast_send(&bc)) {
              PRINTF("dbg: Remote RALL request sent\n");

              state = ST_RALL_REMOTE_REQ;

              /* if a MSG_RALL_REPORT is not received in time, inform the wrapper */
              ctimer_set(&rall_remote_init_timer, RALL_REPORT_WAIT, rall_remote_init_to_cb, NULL);

              OK();
            }
            else {
              state = ST_IDLE;
              printf("DALL %02X%02X\n", current_init.u8[0], current_init.u8[1]); // failure
            }
          }
        }
        else {
          printf("DALL %02X%02X\n", current_init.u8[0], current_init.u8[1]); // BUSY
        }
      }
      else if (decode_ambc(data, input_len, &rall_init, dst_list, &num_dst,
      adjm_distances_list, &current_adjm_msg, &total_adjm_msg)) {
        if (state == ST_IDLE) {
          int i;

          /* copy information in the MSG_ADJM packet */
          packetbuf_clear();
          packetman_setpayload(&rall_init, sizeof(rall_init));
          packetman_setpayload(&current_adjm_msg, sizeof(current_adjm_msg));
          packetman_setpayload(&total_adjm_msg, sizeof(total_adjm_msg));
          packetman_setpayload(&num_dst, sizeof(num_dst));
          for (i = 0; i < num_dst; i++) {
            packetman_setpayload(&dst_list[i], sizeof(*dst_list));
            packetman_setpayload(&adjm_distances_list[i], sizeof(*adjm_distances_list));
          }
          packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_broadcast);

          /* add header with message type MSG_ADJM */
          static message_header header;
          header.type = MSG_ADJM;
          if(!packetman_sethdr(&header, sizeof(header))) {
            CERR();
            continue;
          }

          /* transmit */
          if (broadcast_send(&bc)) {
            PRINTF("dbg: ADJM (%hhu of %hhu) sent\n", current_adjm_msg, total_adjm_msg);

            OK();
          }
          else {
            printf("AMBC FAIL\n");
          }
        }
        else {
          printf("AMBC FAIL\n");
        }
      }
      else {
        PRINTF("dbg: Command not recognized\n");

        CERR();
      }
    }
    else { // should not happen
      // XXX: state change?
      PRINTF("dbg: Err, echo (%d) '%s', state %d\n", input_len, (char*)data, state);
    }
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(cubes_ranging, ev, data)
{
  static uint8_t attempts;

  PROCESS_BEGIN();

  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);

    attempts = 0;

    while (1) {
      PRINTF("dbg: RNGE %04X, attempt %d\n", current_peer.u16, attempts);

      if (attempts >= RNG_MAX_ATTEMPTS) {
        state = ST_IDLE;
        printf("DIST FAIL %02X%02X\n",
            current_peer.u8[0], current_peer.u8[1]);
        break;
      }
      attempts++;

      if (!range_with(&current_peer, DW1000_RNG_DS)) continue;
      PROCESS_YIELD_UNTIL(ev == ranging_event);

      if (((ranging_data_t*)data)->status) {
        state = ST_IDLE;
        printf("DIST _OK_ %04X %.3f\n", current_peer.u16, ((ranging_data_t*)data)->distance);
        break;
      }
    }
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(cubes_ranging_all, ev, data)
{
  static uint8_t attempts;
  static int i;

  PROCESS_BEGIN();

  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);

    /* attempt ranging with all listed destinations */
    for (i = 0; i < num_dst; i++) {

      current_peer.u16 = dst_list[i];
      attempts = 0;

      while (1) {
        PRINTF("dbg: RALL %04X, attempt %d\n", current_peer.u16, attempts);

        if (attempts >= RNG_MAX_ATTEMPTS) {
          distances_list[i] = NAN;

          PRINTF("dbg: Max attempts, %04X (idx %d) set to %lf\n",
            current_peer.u16, i, distances_list[i]);

          break;
        }
        attempts++;

        if (!range_with(&current_peer, DW1000_RNG_DS)) continue;
        PROCESS_YIELD_UNTIL(ev == ranging_event);

        if (((ranging_data_t*)data)->status) {
          distances_list[i] = ((ranging_data_t*)data)->distance;
          if (distances_list[i] < 0) distances_list[i] = 0;

          PRINTF("dbg: Ranging ok, %04X (idx %d) set to %lf\n",
            current_peer.u16, i, distances_list[i]);

          break;
        }
        else {
          distances_list[i] = -1;
        }
      }
    }

    /* if the RALL was requested by our wrapper */
    if (state == ST_RALL) {
      static char init_encoded[LINKADDR_SIZE * 2 + 1];
      static char addr_encoded[LINKADDR_SIZE * 2 + 1];
      uint16_t enc_len;

      enc_len = encode_hex((char*)&rall_init, LINKADDR_SIZE, init_encoded);
      init_encoded[enc_len] = '\0';
      *buffer = '\0';
      for (i = 0; i < num_dst; i++) {
        if (!isnan(distances_list[i])) {
          PRINTF("dbg: Adding to DALL, %04X (idx %d) set to %lf\n",
            dst_list[i], i, distances_list[i]);

          enc_len = encode_hex((char*)&dst_list[i], LINKADDR_SIZE, addr_encoded);
          addr_encoded[enc_len] = '\0';
          snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer) - 1, " %s %.03f", addr_encoded, distances_list[i]);
        }
      }

      /* clear state and inform the wrapper */
      state = ST_IDLE;
      num_dst = 0;
      printf("DALL %s%s\n", init_encoded, buffer);
    }
    /* if the RALL was requested remotely by another node */
    else if (state == ST_RALL_REMOTE_INIT) {
      static uint16_t num_dst_successful;

      PRINTF("dbg: Sending remote RALL reply\n");

      /* copy the node ids followed by distance in the MSG_RALL_REPORT packet */
      num_dst_successful = 0;
      packetbuf_clear();
      for (i = 0; i < num_dst; i++) {
        if (distances_list[i] >= 0) num_dst_successful++;
      }
      packetman_setpayload(&num_dst_successful, sizeof(num_dst_successful));
      for (i = 0; i < num_dst; i++) {
        if (distances_list[i] >= 0) {
          PRINTF("dbg: Adding to REPORT, %04X (idx %d) set to %lf\n",
            dst_list[i], i, distances_list[i]);

          packetman_setpayload(&dst_list[i], sizeof(*dst_list));
          packetman_setpayload(&distances_list[i], sizeof(*distances_list));
        }
      }
      packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &rall_requester);

      /* add header with message type MSG_RALL_REPORT */
      static message_header header;
      header.type = MSG_RALL_REPORT;
      if(packetman_sethdr(&header, sizeof(header))) {

        /* clear state and transmit */
        state = ST_IDLE;
        num_dst = 0;
        broadcast_send(&bc);
      }
    }
  }

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/

static void reply_cb(void* ptr) {
  packetbuf_clear(); // send an empty packet
  unicast_send(&beacon_uc, &reply_to);
  replying = 0;
}

static void
beacon_recv_bc(struct broadcast_conn *c, const linkaddr_t *from) {
  PRINTF("beacon from 0x%02x%02x\n", from->u8[0], from->u8[1]);
  printf("RECV %02X%02X 0000\n",
      from->u8[0], from->u8[1]);

  if (!should_respond)
    return; // we are not responding

  if (replying)
    return; // ignore beacons before we reply to the previous one

  replying = 1;
  reply_to = *from;
  ctimer_set(
      &reply_timer,
      random_rand() % BEACON_MAX_RESPONSE_DELAY,
      reply_cb, NULL);
}

static void
beacon_recv_uc(struct unicast_conn *c, const linkaddr_t *from) {
  printf("RECV %02X%02X 0000\n",
      from->u8[0], from->u8[1]);
}

/*---------------------------------------------------------------------------*/

static void
beacon_sent_uc(struct unicast_conn *c, int status, int num_tx) {
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(linkaddr_cmp(dest, &linkaddr_null)) {
    return;
  }
  PRINTF("beacon reply to 0x%02x%02x: status %d num_tx %d\n",
      dest->u8[0], dest->u8[1], status, num_tx);
}

void discovery_init(uint8_t _should_respond) {
  should_respond = _should_respond;
  discovery_active = 1;
  broadcast_open(&beacon_bc, CUBES_BEACON_CHANNEL, &beacon_broadcast_callbacks);
  unicast_open(&beacon_uc, CUBES_BEACON_RESPONSE_CHANNEL, &beacon_unicast_callbacks);
  process_start(&cubes_discovery, NULL);
}

void discovery_beacon() {
  packetbuf_clear(); // send an empty packet
  broadcast_send(&beacon_bc);
}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(cubes_discovery, ev, data) {
  static struct etimer timer;

  PROCESS_BEGIN();

  while(1){
    while (!discovery_active) {
      etimer_set(&timer, CLOCK_SECOND);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }

    while (discovery_active) {
      if (state == ST_IDLE) {
        discovery_beacon();
      }
      etimer_set(&timer, BEACON_PERIOD);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    }
  }
  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
