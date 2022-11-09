#include "contiki.h"
#include "lib/random.h"
#include "leds.h"
#include "net/netstack.h"
#include "net/rime/rime.h"
#include <stdio.h>
#include "dw1000.h"
#include "dw1000-ranging.h"
#include "core/net/linkaddr.h"
#include "serial-line.h"
#include "net/packetbuf.h"

#include "cubes_common.h"

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) do {} while (0)
#endif


/*
 * - Serial protocol specification ------------------------------------
 * Everything apart from the distance is in HEX and big-endinan.
 * Distance is a decimal fraction in meters (XXX maybe change this).
 *
 * - Requests ---------------------------------------------------------
 * SEND dest len_ data
 * RNGE dest
 * RALL init nodelist
 * REST
 * POFF
 * PON_
 * ADDR
 *
 * - Status response --------------------------------------------------
 * [SEND, RNGE, RALL] [_OK_, BUSY, FAIL, OFF_]
 * REST _OK_
 * POFF _OK_
 * PON_ _OK_
 *
 * - Indications ------------------------------------------------------
 * SENT [_OK_, NO_A]
 * RECV src_ len_ data
 * DIST [_OK_ init distance, FAIL init]
 * DALL init node-distance-list
 * CERR 'original_line'
 */

static enum state {
  ST_OFF,
  ST_IDLE,
  ST_SENDING,
  ST_RANGING
} state;

/* The remote node we are currently working with */
linkaddr_t current_peer;

#define IS(cmd) (strncmp((char*)input, #cmd, 4) == 0)

#define _EXTR_CMD() ((uint8_t*)data)[0], ((uint8_t*)data)[1], ((uint8_t*)data)[2], ((uint8_t*)data)[3]
#define _PRINT_ERR(resp) printf("%c%c%c%c " #resp "\n", _EXTR_CMD())
#define BUSY() _PRINT_ERR(BUSY)
#define FAIL() _PRINT_ERR(FAIL)
#define OK() _PRINT_ERR(_OK_)
#define CERR() printf("CERR '%s'\n", (char*)data)


#define MAX_RAW_BUF_LEN 128
#define MAX_ENCODED_BUF_LEN MAX_RAW_BUF_LEN*2

char buffer[MAX_ENCODED_BUF_LEN + 1]; // +1 for zero-termination

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

int decode_addr(const char* input, uint16_t in_len) {
  if (in_len != 4 || !IS(ADDR))
    return 0;

  return 1;
}

const linkaddr_t linkaddr_broadcast = {{ 0xff, 0xff }};
#define CUBES_RIME_CHANNEL 150

// Using broadcast because unicast of Rime doesn't support broadcast.
static struct broadcast_conn bc;

static void
recv(struct broadcast_conn *c, const linkaddr_t* from) {

  const linkaddr_t* to = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);

  if (linkaddr_cmp(to, &linkaddr_node_addr) || to->u16 == 0 || to->u16 == 0xFFFF) {
    uint16_t raw_len = packetbuf_datalen();

    if (raw_len > MAX_RAW_BUF_LEN)
      return;

    uint16_t enc_len = encode_hex((char*)packetbuf_dataptr(), raw_len, buffer);
    buffer[enc_len] = 0; // for printf
    printf("RECV %02X%02X %04X %s\n",
        from->u8[0], from->u8[1],
        raw_len,
        buffer);
  }
}

static void
sent(struct broadcast_conn *c, int status, int num_tx) {
  if (state != ST_SENDING)
    return;
  state = ST_IDLE;

  // TODO: report also FAIL for unspecified error?
  if (status == MAC_TX_OK)
    printf("SENT _OK_\n");
  else
    printf("SENT NO_A\n");
  //PRINTF("%d\n", num_tx);
}

static const struct broadcast_callbacks broadcast_callbacks = {recv, sent};
static const struct packetbuf_attrlist attributes[] = {
  {PACKETBUF_ADDR_RECEIVER, PACKETBUF_ADDRSIZE},
  BROADCAST_ATTRIBUTES
  PACKETBUF_ATTR_LAST
};

PROCESS(cubes_main, "Cubes module main process");
AUTOSTART_PROCESSES(&cubes_main);

PROCESS_THREAD(cubes_main, ev, data)
{
  static int input_len;

  PROCESS_BEGIN();
  broadcast_open(&bc, CUBES_RIME_CHANNEL, &broadcast_callbacks);
  channel_set_attributes(CUBES_RIME_CHANNEL, attributes);

  state = ST_IDLE;

  while(1) {
    PROCESS_WAIT_EVENT();
    if (ev == serial_line_event_message) {
      input_len = strlen(data);
      PRINTF("dbg: echo %d: '%s'\n", input_len, (char*)data);

      static uint16_t dst;
      static uint16_t  datalen;

      if (input_len < 4) {
        CERR();
      }
      else if (decode_send((char*)data, input_len, &dst, &datalen, buffer)) {
        //PRINTF("send to %x len %d\n", dst, datalen);
        if (state == ST_IDLE) {

          if (dst == 0xFFFF) // broadcast addr in Rime is 0
            dst = 0;

          packetbuf_clear();
          packetbuf_copyfrom(buffer, datalen);
          current_peer.u16 = dst;
          packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &current_peer);
          if (broadcast_send(&bc)) {
            state = ST_SENDING;
            OK();
          }
          else {
            FAIL();
          }
        }
        else {
          BUSY();
        }
      }
      else if (decode_range(data, input_len, &dst)) {
        //PRINTF("range with %x\n", dst);
        if (state == ST_IDLE) {
          if (dst == 0xFFFF || dst == 0) {
            CERR();
          }
          else {
            current_peer.u16 = dst;
            if (range_with(&current_peer, DW1000_RNG_DS)) {
              state = ST_RANGING;
              OK();
            }
            else {
              FAIL();
            }
          }
        }
        else {
          BUSY();
        }
      }
      else if (decode_addr(data, input_len)) {
        printf("ADDR %02X%02X\n", linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1]);
      }
      else {
        CERR();
      }
    }
    else if (ev == ranging_event) {
      if (state == ST_RANGING) {
        state = ST_IDLE;
        if (((ranging_data_t*)data)->status) {
          printf("DIST _OK_ %02X%02X %f\n", current_peer.u8[0], current_peer.u8[1],
              ((ranging_data_t*)data)->distance);
        }
        else
          printf("DIST FAIL %02X%02X\n", current_peer.u8[0], current_peer.u8[1]);
      }
    }
    else { // should not happen
      PRINTF("dbg: stale ranging response\n");
    }
  }

  PROCESS_END();
}
