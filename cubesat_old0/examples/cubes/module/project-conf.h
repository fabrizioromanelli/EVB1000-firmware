#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#include "deca_device_api.h"

/*-- 6.8Mbps configuration ------------------------------------------*/


#define DW1000_CONF_CHANNEL        4
#define DW1000_CONF_PRF            DWT_PRF_64M
#define DW1000_CONF_PLEN           DWT_PLEN_128
#define DW1000_CONF_PAC            DWT_PAC8
#define DW1000_CONF_SFD_MODE       0
#define DW1000_CONF_DATA_RATE      DWT_BR_6M8
#define DW1000_CONF_PHR_MODE       DWT_PHRMODE_STD
#define DW1000_CONF_PREAMBLE_CODE  17
#define DW1000_CONF_SFD_TIMEOUT    (129 + 8 - 8)


/*-- 110kbps configuration ------------------------------------------*/
/*
#define DW1000_CONF_CHANNEL        2
#define DW1000_CONF_PRF            DWT_PRF_64M
#define DW1000_CONF_PLEN           DWT_PLEN_1024
#define DW1000_CONF_PAC            DWT_PAC32
#define DW1000_CONF_SFD_MODE       1
#define DW1000_CONF_DATA_RATE      DWT_BR_110K
#define DW1000_CONF_PHR_MODE       DWT_PHRMODE_STD
#define DW1000_CONF_PREAMBLE_CODE  9
#define DW1000_CONF_SFD_TIMEOUT    (1025 + 64 - 32)
*/
/*-------------------------------------------------------------------*/

#define IEEE802154_CONF_PANID 0xA0B0
#define DW1000_CONF_FRAMEFILTER 0 // otherwise RIME bcasts do not work 

#endif /* PROJECT_CONF_H_ */
