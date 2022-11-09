#pragma once

#include "config.h"

// Distance row completed, sending token to another node.
// The protocol is:
// byte 0: NEXT NODE  (uint16_t)
// byte 2: TIMESTAMP  (uint32_t)
// byte 6: DISTANCE 1 (uint16_t) 
//	...
// max 30 nodes

#define MIN_VALID_BUFFER 6 //minimum valid buffer should contain at least node_to and timestamp
#define SEND_ACK_LEN 6
#define MAX_BROADCAST_LEN 56

#ifdef TRANSMISSION_LE
#define GET_NEXT_NODE(buff) 		(((char *)buff)[0] | ((char *)buff)[1] << 8)
#define GET_TIMESTAMP(buff) 		(((char *)buff)[2] | ((char *)buff)[3] << 8 | ((char *)buff)[4] << 16 | ((char *)buff)[5] << 24)
#define GET_DISTANCE(buff, idx) 	(((char *)buff)[6 + idx] | ((char *)buff)[7 + idx] << 8)

#define SET_NEXT_NODE(buff, node) 	((char *)buff)[0] = (char) node; ((char *) buff)[1] = (char) (node >> 8)
#define SET_TIMESTAMP(buff, time) 	((char *)buff)[2] = (char) time; ((char *) buff)[3] = (char) (time >> 8); ((char *) buff)[4] = (char) (time >> 16); ((char *) buff)[5] = (char) (time >> 24)
#define SET_DISTANCE(buff, dist, idx) 	((char *)buff)[6 + idx] = (char) dist; ((char *) buff)[7 + idx] = (char) (dist >> 8)
#endif

#ifdef TRANSMISSION_BE
#define NEXT_NODE(buff) 		(((char *)buff)[1] | ((char *)buff)[0] << 8)
#define TIMESTAMP(buff) 		(((char *)buff)[5] | ((char *)buff)[4] << 8 | ((char *)buff)[3] << 16 | ((char *)buff)[2] << 24)
#define GET_DISTANCE(buff, idx) 	(((char *)buff)[7 + idx] | ((char *)buff)[6 + idx] << 8)

#define SET_NEXT_NODE(buff, node) 	((char *)buff)[1] = (char) node; ((char *) buff)[0] = (char) (node >> 8)
#define SET_TIMESTAMP(buff, time) 	((char *)buff)[5] = (char) time; ((char *) buff)[4] = (char) (time >> 8); ((char *) buff)[3] = (char) (time >> 16); ((char *) buff)[2] = (char) (time >> 24)
#define SET_DISTANCE(buff, dist, idx) 	((char *)buff)[7 + idx] = (char) dist; ((char *) buff)[6 + idx] = (char) (dist >> 8)
#endif

uint8_t sendack[SEND_ACK_LEN] = {'\0', '\0', 'a', 'c', 'k', '\0'};

static int check_ack(uint8_t * buff, int bufflen) {
	if(bufflen == SEND_ACK_LEN) {
		return buff[0] == 0 && buff[1] == 0 && buff[2] == 'a' && buff[3] == 'c' && buff[4] == 'k' && buff[5] == 0;
	}
	return 0;
}

