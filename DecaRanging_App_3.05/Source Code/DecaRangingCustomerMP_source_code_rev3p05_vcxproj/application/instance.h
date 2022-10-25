/*! ----------------------------------------------------------------------------
 * @file	instance.h 
 * @brief	DecaWave header for application level instance
 *		    
 *
 * @attention
 *
 * Copyright 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */

#ifndef _INSTANCE_H_
#define _INSTANCE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "deca_types.h"
#include "deca_device_api.h"

// Select whether we deliver sounding data (accumulator data)
#define DECA_SUPPORT_SOUNDING   (1)                     // NB: if enabled this needs extra buffer memory and processing power.

// Select whether we generate a log file.
#define DECA_LOG_ENABLE   (1)                           // set to 0 to disable loging

// Select whether software needs to copy accumulator data, e.g. for diagnostic graphing
#define DECA_KEEP_ACCUMULATOR   (1)                     // set to zero if not required

// Select whether we support logging accumulator data to a file.
#define DECA_ACCUM_LOG_SUPPORT  (1)                     // set to 0 to remove support

// Select whether software reads accumulator for bad frames
#define DECA_BADF_ACCUMULATOR   (1)                     // set to zero if not required

// Select whether software logs errored/bad frames
#define DECA_ERROR_LOGGING   (1)                     // set to zero if not required

#if DECA_SUPPORT_SOUNDING==0
#if DECA_KEEP_ACCUMULATOR==1
#error 'Code expects DECA_SUPPORT_SOUNDING set to keep accumulator data'
#endif
#if DECA_ACCUM_LOG_SUPPORT==1
#error 'Code expects DECA_SUPPORT_SOUNDING set to keep accumulator data for logging'
#endif
#endif

/******************************************************************************************************************
********************* NOTES on DW (MP) features/options ***********************************************************
*******************************************************************************************************************/

// DEEP_SLEEP support has been removed in PC application due to the use of long response delays.
// When using long response delays, sleeping between frames causes crystal oscillations on wake-up to have a significant
// impact on calculated ToF. Please report to ARM source code to have an exemple of sleep support implememtation.

#define CORRECT_RANGE_BIAS  (1)     // Compensate for small bias due to uneven accumulator growth at close up high power

/******************************************************************************************************************
*******************************************************************************************************************
*******************************************************************************************************************/

// Number of milliseconds in a day.
#define DAY_TO_MS (24 * 60 * 60 * 1000)

#define NUM_INST            1
#define SPEED_OF_LIGHT      (299702547.0)     // in m/s in air
#define MASK_40BIT			(0x00FFFFFFFFFF)  // MP counter is 40 bits
#define MASK_TXDTS			(0x00FFFFFFFE00)  //The TX timestamp will snap to 8 ns resolution - mask lower 9 bits. 


#define USING_64BIT_ADDR (1) //when set to 0 - the DecaRanging application will use 16-bit addresses

#define SIG_RX_BLINK			7		// Received ISO EUI 64 blink message
#define SIG_RX_UNKNOWN			99		// Received an unknown frame

// Existing frames type in ranging process.
enum
{
    BLINK = 0,
    RNG_INIT,
    POLL,
    RESP,
    FINAL,
    FRAME_TYPE_NB
};

//Fast 2WR function codes
#define RTLS_DEMO_MSG_RNG_INIT				(0x20)			// Ranging initiation message
#define RTLS_DEMO_MSG_TAG_POLL              (0x21)          // Tag poll message
#define RTLS_DEMO_MSG_ANCH_RESP             (0x10)          // Anchor response to poll
#define RTLS_DEMO_MSG_TAG_FINAL             (0x29)          // Tag final massage back to Anchor (0x29 because of 5 byte timestamps needed for PC app)

//lengths including the Decaranging Message Function Code byte
#define TAG_POLL_MSG_LEN                    1                // FunctionCode(1),
#define ANCH_RESPONSE_MSG_LEN               9               // FunctionCode(1), RespOption (1), OptionParam(2), Measured_TOF_Time(5)
#define TAG_FINAL_MSG_LEN                   16              // FunctionCode(1), Poll_TxTime(5), Resp_RxTime(5), Final_TxTime(5)
#define RANGINGINIT_MSG_LEN					7				// FunctionCode(1), Tag Address (2), Response Time (2) * 2

#define MAX_MAC_MSG_DATA_LEN                (TAG_FINAL_MSG_LEN) //max message len of the above

#define STANDARD_FRAME_SIZE         127

#define ADDR_BYTE_SIZE_L            (8)
#define ADDR_BYTE_SIZE_S            (2)

#define FRAME_CONTROL_BYTES         2
#define FRAME_SEQ_NUM_BYTES         1
#define FRAME_PANID                 2
#define FRAME_CRC					2
#define FRAME_SOURCE_ADDRESS_S        (ADDR_BYTE_SIZE_S)
#define FRAME_DEST_ADDRESS_S          (ADDR_BYTE_SIZE_S)
#define FRAME_SOURCE_ADDRESS_L        (ADDR_BYTE_SIZE_L)
#define FRAME_DEST_ADDRESS_L          (ADDR_BYTE_SIZE_L)
#define FRAME_CTRLP					(FRAME_CONTROL_BYTES + FRAME_SEQ_NUM_BYTES + FRAME_PANID) //5
#define FRAME_CRTL_AND_ADDRESS_L    (FRAME_DEST_ADDRESS_L + FRAME_SOURCE_ADDRESS_L + FRAME_CTRLP) //21 bytes for 64-bit addresses)
#define FRAME_CRTL_AND_ADDRESS_S    (FRAME_DEST_ADDRESS_S + FRAME_SOURCE_ADDRESS_S + FRAME_CTRLP) //9 bytes for 16-bit addresses)
#define FRAME_CRTL_AND_ADDRESS_LS	(FRAME_DEST_ADDRESS_L + FRAME_SOURCE_ADDRESS_S + FRAME_CTRLP) //15 bytes for 1 16-bit address and 1 64-bit address)
#define MAX_USER_PAYLOAD_STRING_LL     (STANDARD_FRAME_SIZE-FRAME_CRTL_AND_ADDRESS_L-TAG_FINAL_MSG_LEN-FRAME_CRC) //127 - 21 - 16 - 2 = 88
#define MAX_USER_PAYLOAD_STRING_SS     (STANDARD_FRAME_SIZE-FRAME_CRTL_AND_ADDRESS_S-TAG_FINAL_MSG_LEN-FRAME_CRC) //127 - 9 - 16 - 2 = 100
#define MAX_USER_PAYLOAD_STRING_LS     (STANDARD_FRAME_SIZE-FRAME_CRTL_AND_ADDRESS_LS-TAG_FINAL_MSG_LEN-FRAME_CRC) //127 - 15 - 16 - 2 = 94

//NOTE: the user payload assumes that there are only 88 "free" bytes to be used for the user message (it does not scale according to the addressing modes)
#define MAX_USER_PAYLOAD_STRING	MAX_USER_PAYLOAD_STRING_LL

// Total frame lengths.
#if (USING_64BIT_ADDR == 1)
    #define RNG_INIT_FRAME_LEN_BYTES (RANGINGINIT_MSG_LEN + FRAME_CRTL_AND_ADDRESS_L + FRAME_CRC)
    #define POLL_FRAME_LEN_BYTES (TAG_POLL_MSG_LEN + FRAME_CRTL_AND_ADDRESS_L + FRAME_CRC)
    #define RESP_FRAME_LEN_BYTES (ANCH_RESPONSE_MSG_LEN + FRAME_CRTL_AND_ADDRESS_L + FRAME_CRC)
    #define FINAL_FRAME_LEN_BYTES (TAG_FINAL_MSG_LEN + FRAME_CRTL_AND_ADDRESS_L + FRAME_CRC)
#else
    #define RNG_INIT_FRAME_LEN_BYTES (RANGINGINIT_MSG_LEN + FRAME_CRTL_AND_ADDRESS_LS + FRAME_CRC)
    #define POLL_FRAME_LEN_BYTES (TAG_POLL_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC)
    #define RESP_FRAME_LEN_BYTES (ANCH_RESPONSE_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC)
    #define FINAL_FRAME_LEN_BYTES (TAG_FINAL_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC)
#endif

#define BLINK_FRAME_CONTROL_BYTES       1
#define BLINK_FRAME_SEQ_NUM_BYTES       1
#define BLINK_FRAME_CRC					2
#define BLINK_FRAME_SOURCE_ADDRESS      8
#define BLINK_FRAME_CTRLP				(BLINK_FRAME_CONTROL_BYTES + BLINK_FRAME_SEQ_NUM_BYTES) //2
#define BLINK_FRAME_CRTL_AND_ADDRESS    (BLINK_FRAME_SOURCE_ADDRESS + BLINK_FRAME_CTRLP) //10 bytes
#define BLINK_FRAME_LEN_BYTES           (BLINK_FRAME_CTRLP + BLINK_FRAME_CRTL_AND_ADDRESS)

#define ANCHOR_LIST_SIZE			(4)
#define TAG_LIST_SIZE				(10)

#define BLINK_PERIOD_DEFAULT_MS 1000
#define POLL_PERIOD_DEFAULT_MS 1000

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// NOTE: the maximum RX timeout is ~ 65ms
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

#define INST_DONE_WAIT_FOR_NEXT_EVENT		1	//this signifies that the current event has been processed and instance is ready for next one
#define INST_DONE_WAIT_FOR_NEXT_EVENT_TO	2	//this signifies that the current event has been processed and that instance is waiting for next one with a timeout
												//which will trigger if no event coming in specified time
#define INST_DONE_WAIT_FOR_NEXT_EVENT_TOREP 3
#define INST_NOT_DONE_YET					0	//this signifies that the instance is still processing the current event

// Function code byte offset (valid for all message types).
#define FCODE                               0               // Function code is 1st byte of messageData
// Final message byte offsets.
#define PTXT                                1
#define RRXT                                6
#define FTXT                                11
// Length of ToF value in report message. Can be used as offset to put up to
// 4 ToF values in the report message.
#define TOFR                                4
// Anchor response byte offsets.
#define RES_R1                              1               // Response option octet 0x02 (1),
#define RES_R2                              2               // Response option parameter 0x00 (1) - used to notify Tag that the report is coming
#define RES_R3                              3               // Response option parameter 0x00 (1),
// Ranging init message byte offsets. Composed of tag short address, anchor
// response delay and tag response delay.
#define RNG_INIT_TAG_SHORT_ADDR_LO 1
#define RNG_INIT_TAG_SHORT_ADDR_HI 2
#define RNG_INIT_ANC_RESP_DLY_LO 3
#define RNG_INIT_ANC_RESP_DLY_HI 4
#define RNG_INIT_TAG_RESP_DLY_LO 5
#define RNG_INIT_TAG_RESP_DLY_HI 6

// Response delay values coded in ranging init message.
// This is a bitfield composed of:
//   - bits 0 to 14: value
//   - bit 15: unit
#define RESP_DLY_VAL_SHIFT 0
#define RESP_DLY_VAL_MASK 0x7FFF
#define RESP_DLY_UNIT_SHIFT 15
#define RESP_DLY_UNIT_MASK 0x8000

// Response time possible units: microseconds or milliseconds.
#define RESP_DLY_UNIT_US 0
#define RESP_DLY_UNIT_MS 1

// Response delays types present in ranging init message.
enum
{
    RESP_DLY_ANC = 0,
    RESP_DLY_TAG,
    RESP_DLY_NB
};

// Convert microseconds to symbols, float version.
// param  x  value in microseconds
// return  value in symbols.
#define US_TO_SY(x) ((x) / 1.0256)

// Convert microseconds to symbols, integer version.
// param  x  value in microseconds
// return  value in symbols.
// /!\ Due to the the multiplication by 10000, be careful about potential
// values and type for x to avoid overflows.
#define US_TO_SY_INT(x) (((x) * 10000) / 10256)

// Default anchor response delay.
#define ANC_RESP_DLY_DEFAULT_MS 150
// Default tag response delay.
#define TAG_RESP_DLY_DEFAULT_MS 200

// Delay between blink reception and ranging init message. This is the same for
// all modes.
#define RNG_INIT_REPLY_DLY_MS (150)

// Reception start-up time, in symbols.
#define RX_START_UP_SY 16

typedef enum instanceModes{LISTENER, TAG, ANCHOR, TAG_TDOA, NUM_MODES} INST_MODE;

//Listener = in this mode, the instance only receives frames, does not respond
//Tag = Exchanges DecaRanging messages (Poll-Response-Final) with Anchor and enabling Anchor to calculate the range between the two instances
//Anchor = see above

typedef enum inst_states
{
    TA_INIT, //0

    TA_TXE_WAIT,                //1
    TA_TXPOLL_WAIT_SEND,        //2
    TA_TXFINAL_WAIT_SEND,       //3
    TA_TXRESPONSE_WAIT_SEND,    //4
    TA_TX_WAIT_CONF,            //6

    TA_RXE_WAIT,                //7
    TA_RX_WAIT_DATA,            //8

    TA_SLEEP_DONE,              //9
    TA_TXBLINK_WAIT_SEND,       //10
    TA_TXRANGINGINIT_WAIT_SEND,  //11
	TA_PAUSED
} INST_STATES;


// This file defines data and functions for access to Parameters in the Device
//message structure for Poll, Response and Final message

typedef struct
{
    uint8 frameCtrl[2];                         	//  frame control bytes 00-01
    uint8 seqNum;                               	//  sequence_number 02
    uint8 panID[2];                             	//  PAN ID 03-04
    uint8 destAddr[ADDR_BYTE_SIZE_L];             	//  05-12 using 64 bit addresses
    uint8 sourceAddr[ADDR_BYTE_SIZE_L];           	//  13-20 using 64 bit addresses
    uint8 messageData[MAX_USER_PAYLOAD_STRING_LL] ; //  22-124 (application data and any user payload)
    uint8 fcs[2] ;                              	//  125-126  we allow space for the CRC as it is logically part of the message. However ScenSor TX calculates and adds these bytes.
} srd_msg_dlsl ;

typedef struct
{
    uint8 frameCtrl[2];                         	//  frame control bytes 00-01
    uint8 seqNum;                               	//  sequence_number 02
    uint8 panID[2];                             	//  PAN ID 03-04
    uint8 destAddr[ADDR_BYTE_SIZE_S];             	//  05-06
    uint8 sourceAddr[ADDR_BYTE_SIZE_S];           	//  07-08
    uint8 messageData[MAX_USER_PAYLOAD_STRING_SS] ; //  09-124 (application data and any user payload)
    uint8 fcs[2] ;                              	//  125-126  we allow space for the CRC as it is logically part of the message. However ScenSor TX calculates and adds these bytes.
} srd_msg_dsss ;

typedef struct
{
    uint8 frameCtrl[2];                         	//  frame control bytes 00-01
    uint8 seqNum;                               	//  sequence_number 02
    uint8 panID[2];                             	//  PAN ID 03-04
    uint8 destAddr[ADDR_BYTE_SIZE_L];             	//  05-12 using 64 bit addresses
    uint8 sourceAddr[ADDR_BYTE_SIZE_S];           	//  13-14
    uint8 messageData[MAX_USER_PAYLOAD_STRING_LS] ; //  15-124 (application data and any user payload)
    uint8 fcs[2] ;                              	//  125-126  we allow space for the CRC as it is logically part of the message. However ScenSor TX calculates and adds these bytes.
} srd_msg_dlss ;

typedef struct
{
    uint8 frameCtrl[2];                         	//  frame control bytes 00-01
    uint8 seqNum;                               	//  sequence_number 02
    uint8 panID[2];                             	//  PAN ID 03-04
    uint8 destAddr[ADDR_BYTE_SIZE_S];             	//  05-06
    uint8 sourceAddr[ADDR_BYTE_SIZE_L];           	//  07-14 using 64 bit addresses
    uint8 messageData[MAX_USER_PAYLOAD_STRING_LS] ; //  15-124 (application data and any user payload)
    uint8 fcs[2] ;                              	//  125-126  we allow space for the CRC as it is logically part of the message. However ScenSor TX calculates and adds these bytes.
} srd_msg_dssl ;

//12 octets for Minimum IEEE ID blink
typedef struct
{
    uint8 frameCtrl;                         		//  frame control bytes 00
    uint8 seqNum;                               	//  sequence_number 01
    uint8 tagID[BLINK_FRAME_SOURCE_ADDRESS];        //  02-09 64 bit address
    uint8 fcs[2] ;                              	//  10-11  we allow space for the CRC as it is logically part of the message. However ScenSor TX calculates and adds these bytes.
} iso_IEEE_EUI64_blink_msg ;

//18 octets for IEEE ID blink with Temp and Vbat values
typedef struct
{
    uint8 frameCtrl;                         		//  frame control bytes 00
    uint8 seqNum;                               	//  sequence_number 01
    uint8 tagID[BLINK_FRAME_SOURCE_ADDRESS];           			//  02-09 64 bit addresses
	uint8 enchead[2];								//  10-11 2 bytes (encoded header and header extension)
	uint8 messageID;								//  12 message ID (0xD1) - DecaWave message
	uint8 temp;										//  13 temperature value
	uint8 vbat;										//  14 voltage value
	uint8 gpio;										//  15 gpio status
    uint8 fcs[2] ;                              	//  16-17  we allow space for the CRC as it is logically part of the message. However ScenSor TX calculates and adds these bytes.
} iso_IEEE_EUI64_blinkdw_msg ;

typedef struct
{
    uint8 frameCtrl[2];                         	//  frame control bytes 00-01
    uint8 seqNum;                               	//  sequence_number 02
    uint8 fcs[2] ;                              	//  03-04  CRC
} ack_msg ;

typedef struct
{
    uint8 channelNumber ;       // valid range is 1 to 11
    uint8 preambleCode ;        // 00 = use NS code, 1 to 24 selects code
    uint8 pulseRepFreq ;        // NOMINAL_4M, NOMINAL_16M, or NOMINAL_64M
    uint8 dataRate ;            // DATA_RATE_1 (110K), DATA_RATE_2 (850K), DATA_RATE_3 (6M81)
    uint8 preambleLen ;         // values expected are 64, (128), (256), (512), 1024, (2048), and 4096
    uint8 pacSize ;
    uint8 nsSFD ;
    //uint16 sfdTO;  //!< SFD timeout value (in symbols) e.g. preamble length (128) + SFD(8) - PAC + some margin ~ 135us... DWT_SFDTOC_DEF; //default value
} instanceConfig_t ;

typedef struct
{
	uint32 icid;

	dwt_rxdiag_t diag;

#if DECA_LOG_ENABLE==1
    int         accumLogging ;                                // log data to a file, used to indicate that we are currenty logging (file is open)
	FILE        *accumLogFile ;                               // file
#endif

} devicelogdata_t ;

/******************************************************************************************************************
**************** DATA STRUCTURES USED IN PC APPLICATION LOGGING/DISPLAYING ****************************************************************
*******************************************************************************************************************/
#define MAX_RANGE_ELEMENTS		(100)
#define MAX_ACCUMULATOR_ELEMENTS	(1016*2)
#define RX_ACCD_MAX_LEN					(9000)            // max accumulator length in bytes
typedef struct
{
	uint8       spi_read_padding ;
    uint8       dummy ;                                 // extra 1 as first byte is dummy due to internal memory access delay
    uint8       accumRawData[RX_ACCD_MAX_LEN] ;         // data bytes
} accBuff_t ;

typedef struct deca_buffer
{
	uint16      accumLength ;                       // length of data actually present (number of complex samples)
#if DECA_SUPPORT_SOUNDING==1
    accBuff_t   *accumData ;						// data bytes
#endif
} deca_buffer_t ;

typedef struct {        int16   real ;
                        int16   imag ;  } complex16 ;

typedef struct
{
#if (DECA_KEEP_ACCUMULATOR==1)
    complex16 accvals[MAX_ACCUMULATOR_ELEMENTS] ;
	int32  magvals [MAX_ACCUMULATOR_ELEMENTS] ;
#else
	complex16 *accvals ;
	int32 *magvals;
#endif
    int accumSize ;

    int16 windInd ;
    uint16 preambleSymSeen ;
	uint16 maxGrowthCIR ;
    double hwrFirstPath ;
    double totalSNR ;
    double RSL ;
    double totalSNRavg ;
    double RSLavg ;
    double fpSNR ;
    double fpRelAtten ;

	char cp[8];
} accumPlotInfo_t ;


typedef struct
{
	int32  magvals [MAX_RANGE_ELEMENTS] ; //in centimeters
    int arraySize ;

} rangesPlotInfo_t ;

typedef struct
{
    #if DECA_SUPPORT_SOUNDING==1
	#if DECA_KEEP_ACCUMULATOR==1
    uint16					accumLength ;
    complex16				accumCmplx[MAX_ACCUMULATOR_ELEMENTS] ;
	int8                    newAccumData ;                                // flag data present
    int8                    erroredFrame ;                                // flag was errored frame
	//uint8					preambData[1024];
	deca_buffer_t			buff ;
	#endif
	#else
	uint8 dummy;
    #endif
} deviceacclogdata_t ;

// number of lines for status report
#define NUM_STAT_LINES              6           // total number of status lines
#define NUM_BASIC_STAT_LINES        4           // first group of four lines
#define STAT_LINE_MSGCNT            0
#define STAT_LINE_INST              1
#define STAT_LINE_MEAN              2
#define STAT_LINE_LTAVE             3
// #define STAT_LINE_RESP              4
#define STAT_LINE_USER_RX1          4
#define STAT_LINE_USER_RX2          5
// character length of lines for status report
#define STAT_LINE_LENGTH    160
#define STAT_LINE_LONG_LENGTH 1024
#define STAT_LINE_BUF_LENGTH    (STAT_LINE_LENGTH+1) // allow space for NULL string
#define STAT_LINE_MSGBUF_LENGTH    ((STAT_LINE_LONG_LENGTH*2)+1+5) // allow space for NULL string + 10 for (bytes) and *2 because of 2chars per byte printing

#define NUM_MSGBUFF_POW2        5
#define NUM_MSGBUFF_LINES       (1<<NUM_MSGBUFF_POW2)       // NB: power of 2 !!!!!!!!
#define NUM_MSGBUFF_MODULO_MASK (NUM_MSGBUFF_LINES-1)

typedef struct
{
    char scr[NUM_STAT_LINES][STAT_LINE_BUF_LENGTH] ;            // screen layout
	char lmsg_scrbuf[NUM_MSGBUFF_LINES][STAT_LINE_MSGBUF_LENGTH] ;      // lmsg_scrbuf listener message log screen buffer
    int lmsg_last_index ; 
    int changed ;
} statusReport_t ;

typedef struct
{
    double idist ;
    double mean4 ; 
    double mean ; 
    int numAveraged ;
	uint32 lastRangeReportTime ;
} currentRangeingInfo_t ;

/******************************************************************************************************************
*******************************************************************************************************************
*******************************************************************************************************************/

#define MAX_EVENT_NUMBER (4)
//NOTE: Accumulators don't need to be stored as part of the event structure as when reading them only one RX event can happen... 
//the receiver is singly buffered and will stop after a frame is received

typedef struct
{
	uint8  type;			// event type
	uint8  type2;			// holds the event type - does not clear (not used to show if event has been processed)
    uint8  type3;
	uint8  broadcastmsg;	// specifies if the rx message is broadcast message
	uint16 rxLength ;

	uint64 timeStamp ;		// last timestamp (Tx or Rx)
	
	union {
			//holds received frame (after a good RX frame event)
			uint8   frame[STANDARD_FRAME_SIZE];
    		srd_msg_dlsl rxmsg_ll ; //64 bit addresses
			srd_msg_dssl rxmsg_sl ; 
			srd_msg_dlss rxmsg_ls ; 
			srd_msg_dsss rxmsg_ss ; //16 bit addresses
			ack_msg rxackmsg ; //holds received ACK frame 
			iso_IEEE_EUI64_blink_msg rxblinkmsg;
			iso_IEEE_EUI64_blinkdw_msg rxblinkmsgdw;
	}msgu;

}event_data_t ;

#define RTD_MED_SZ          8      // buffer size for mean of 8
#define RTD_STDEV_SZ		50	   // buffer size for STDEV of 50
//TX power configuration structure... 
typedef struct {
                uint8 PGdelay;

				//TX POWER
				//31:24		BOOST_0.125ms_PWR
				//23:16		BOOST_0.25ms_PWR-TX_SHR_PWR
				//15:8		BOOST_0.5ms_PWR-TX_PHR_PWR
				//7:0		DEFAULT_PWR-TX_DATA_PWR
                uint32 txPwr[2]; // 
}tx_struct;

typedef struct
{
    INST_MODE mode;				//instance mode (tag, anchor or listener)

    INST_STATES testAppState ;			//state machine - current state
    INST_STATES nextState ;				//state machine - next state
    INST_STATES previousState ;			//state machine - previous state
    int done ;					//done with the current event/wait for next event to arrive
    
	int pauseRequest ;			//user has paused the application 

	
	//configuration structures
	dwt_config_t    configData ;	//DW1000 channel configuration 
	dwt_txconfig_t  configTX ;		//DW1000 TX power configuration 
	uint16			txantennaDelay ; //DW1000 TX antenna delay
	// "MAC" features 
    uint8 frameFilteringEnabled ;	//frame filtering is enabled

    //timeouts and delays
    int poll_period_ms;
    int blink_period_ms;
    //this is the delay used for the delayed transmit (when sending the ranging init, response, and final messages)
    uint64 rnginitReplyDelay;
    uint64 finalReplyDelay;
    uint64 responseReplyDelay;
    int finalReplyDelay_ms;
    int responseReplyDelay_ms;

    // xx_sy the units are 1.0256 us
    uint32 txToRxDelayAnc_sy;    // this is the delay used after sending a response and turning on the receiver to receive final
    uint32 txToRxDelayTag_sy;    // this is the delay used after sending a poll and turning on the receiver to receive response
    int rnginitW4Rdelay_sy;	// this is the delay used after sending a blink and turning on the receiver to receive the ranging init message

    int fwtoTime_sy;	//this is final message duration (longest out of ranging messages)
    int fwtoTimeB_sy;	//this is the ranging init message duration

    uint32 delayedReplyTime;		// delayed reply time of delayed TX message - high 32 bits

    uint32 rxTimeouts;
    // - not used in the ARM code uint32 responseTimeouts ;

    // Pre-computed frame lengths for frames involved in the ranging process,
    // in microseconds.
    uint32 fl_us[FRAME_TYPE_NB];

    // Response delays configured by user, in milliseconds.
    int anc_resp_dly_ms;
    int tag_resp_dly_ms;

	//message structures used for transmitted messages
#if (USING_64BIT_ADDR == 1)	
	srd_msg_dlsl rng_initmsg ;	// ranging init message (destination long, source long)
    srd_msg_dlsl msg ;			// simple 802.15.4 frame structure (used for tx message) - using long addresses
#else
	srd_msg_dlss rng_initmsg ;  // ranging init message (destination long, source short)
    srd_msg_dsss msg ;			// simple 802.15.4 frame structure (used for tx message) - using short addresses
#endif
	iso_IEEE_EUI64_blink_msg blinkmsg ; // frame structure (used for tx blink message)

	//Tag function address/message configuration 
	uint8   eui64[8];				// devices EUI 64-bit address
	uint16  tagShortAdd ;		    // Tag's short address (16-bit) used when USING_64BIT_ADDR == 0
	uint16  psduLength ;			// used for storing the frame length
    uint8   frame_sn;				// modulo 256 frame sequence number - it is incremented for each new frame transmittion 
	uint16  panid ;					// panid used in the frames

	//union of TX timestamps 
	union {
		uint64 txTimeStamp ;		   // last tx timestamp
		uint64 tagPollTxTime ;		   // tag's poll tx timestamp
	    uint64 anchorRespTxTime ;	   // anchor's reponse tx timestamp
	}txu;

	uint64 anchorRespRxTime ;	    // receive time of response message
	uint64 tagPollRxTime ;          // receive time of poll message


	//application control parameters 
    uint8	wait4ack ;				// if this is set to DWT_RESPONSE_EXPECTED, then the receiver will turn on automatically after TX completion
	uint8	stoptimer;				// stop/disable an active timer
    uint8	instancetimer_en;		// enable/start a timer
    uint32	instancetimer;			// e.g. this timer is used to timeout Tag when in deep sleep so it can send the next poll message
	uint32	instancetimer1;
	uint8	gotTO;					// got timeout event

    //diagnostic counters/data, results and logging 
    int32 tof32;
    int64 tof ;
	double clockOffset ;
	
	int blinkRXcount ;
	int txmsgcount;
	int	rxmsgcount;
	int lateTX;
	int lateRX;

	double  astdevdist[RTD_STDEV_SZ] ;
    double	adist[RTD_MED_SZ] ;
    double	adist4[4] ;
    double	longTermRangeSum ;
    int		longTermRangeCount ;
    int		tofindex ;
	int     tofstdevindex ;
    int		tofcount ;
    uint8   dispFeetInchesEnable ;                         // Display Feet and Inches
    double	idistmax;
    double	idistmin;
    double	idistance ; // instantaneous distance
    double	shortTermMean ;
    double	shortTermMean4 ;
    int		shortTermMeanNumber ;
	uint32	lastReportTime;
	double  RSL;
	double  FSL;

    //logging data/structures used for data/graphical displaying for GUI
    statusReport_t statusrep;
    int last_update ;           // detect changes to status report
    uint8    dispClkOffset;                                // Display Clock Offset

	devicelogdata_t dwlogdata;

	deviceacclogdata_t dwacclogdata ;

	uint8 tagToRangeWith;	//it is the index of the tagList array which contains the address of the Tag we are ranging with
    uint8 tagListLen ;
    uint8 anchorListIndex ;
	uint8 tagList[TAG_LIST_SIZE][8];


	//event queue - used to store DW1000 events as they are processed by the dw_isr/callback functions
    event_data_t dwevent[MAX_EVENT_NUMBER]; //this holds any TX/RX events and associated message data
	event_data_t saved_dwevent; //holds an RX event while the ACK is being sent
    uint8 dweventCntOut;
    uint8 dweventCntIn;
	uint8 dweventPeek;
    uint8 monitor;
    uint32 timeofTx;
} instance_data_t ;


//-------------------------------------------------------------------------------------------------------------
//
//	Functions used in logging/displaying range and status data
//
//-------------------------------------------------------------------------------------------------------------

// function to clear the RX message buffer used for frame display in listener mode
void clearlogmsgbuffer(void) ;
// function to calculate and report the Time of Flight to the GUI/display
void reportTOF(instance_data_t *inst);
// function to clear the ToF data structures
void clearreportTOF(statusReport_t *st);
// clear the status/ranging data 
void instanceclearcounts(void) ;
// copy new data to display
void copylonglinetoscrline(char *scrline);
// copy new data to display
void copylongline2scrline(char *scrla,char *scrlb)  ;                // allow copy to wrap into a 2nd status line
// display new status data
void instancedisplaynewstatusdata(instance_data_t *inst, dwt_deviceentcnts_t *itemp);
// display the rx message data
void instancelogrxdata(instance_data_t *inst, uint8 *buffer, int length);
void instancelogrxblinkdata(instance_data_t *inst, event_data_t *dw_event);
// display the current range info (instantaneous, long term average, mean)
void instancegetcurrentrangeinginfo(currentRangeingInfo_t *info) ;

uint8* instgettaglist(int i);
uint8 instgettaglistlength(void) ;
uint32 instgetblinkrxcount(void) ;
void instcleartaglist(void);
int instgettaglistlen(void);
void instsettagtorangewith(int tagID);
int instaddtagtolist(instance_data_t *inst, uint8 *tagAddr);


// enable reading of the accumulator - used for displaying channel impulse response
void instancesetdisplayfeetinches(int feetInchesEnable) ;           // status report distance in feet and inches
void instancesetdisplayclockoffset(int clockOff) ;					// enable/disable clock offset display
int instancenewplotdata(void) ;                                     // Is there new plot data to display, (returns 1 if yes, otherwise 0)
int instancenewrangedata(void) ;
double instancenewrange(void);
void instancecalculatepower(void);
// sets size and pointer to its accumulator data, and first path values, return string indicates Error Type or OKAY.
const char * instancegetaccumulatordata(accumPlotInfo_t *ap) ;

#if (DECA_ACCUM_LOG_SUPPORT==1) || (DECA_LOG_ENABLE==1)
#include <stdio.h>
#define LOG_SINGLE_ACC      4
#define LOG_ALL_NOACCUM     3
#define LOG_ERR_ACCUM       2
#define LOG_ALL_ACCUM       1
#define LOG_ACCUM_OFF       0
#endif

statusReport_t * getstatusreport(void) ;

#define INST_LOG_OFF    0
#define INST_LOG_ALL    1
#define INST_LOG_ERR    2
#define INST_LOG_ALL_NOACC    3
#define INST_LOG_SINGLE 4

FILE * instanceaccumloghandle(void) ; 
int instanceaccumlogenable(int logEnable) ;     // Returns 0 for success, 1 for failure
void instanceaccumlogflush(void) ;              // flush accumulator log file
int instance_readaccumulatordata(void);
void instancestatusaccumlog(int instance, const char *tag, const char *line);
void processSoundingData(void);
void logSoundingData(int8 errorFlag, int fc, int sn, event_data_t *dw_event);


// read the event counters from the DW1000 - (TX events, RX events, RX Errors, RX timeouts...)
int instancereadevents(dwt_deviceentcnts_t *temp);

// read/write the DW1000 registers
int instancelowlevelreadreg(int regID,int regOffset,unsigned int *retRegValue) ;  // Read low level 32 bit register
int instancelowlevelwritereg(int regID,int regOffset,unsigned int regValue) ;     // Write low level 32 bit register value


//-------------------------------------------------------------------------------------------------------------
//
//	Functions used in driving/controlling the ranging application
//
//-------------------------------------------------------------------------------------------------------------

// close the SPI Cheetah interface  
void instance_close(void);
// called to reset the DW1000 device
void instancedevicesoftreset(void) ;

// Call init, then call config, then call run. call close when finished
// initialise the instance (application) structures and DW1000 device
int instance_init(accBuff_t *buf) ;

// configure the instance and DW1000 device
void instance_config(instanceConfig_t *config) ;
// configure the antenna delays
void instancesetantennadelays(double fdelay) ;                      // delay in nanoseconds
double instancegetantennadelay(int prf);        // returns delay in nanoseconds
void instancesetsnifftimes(int on, int off);

void instancerxon(instance_data_t *inst, int delayed, uint64 delayedReceiveTime);
void inst_processackmsg(instance_data_t *inst, uint8 seqNum);
void inst_processrxtimeout(instance_data_t *inst);

int instancesendpacket(uint16 length, uint8 txmode, uint32 dtime);

// called (periodically or from and interrupt) to process any outstanding TX/RX events and to drive the ranging application
int instance_run(uint32 time) ;       // returns indication of status report change
int testapprun(instance_data_t *inst, int message, uint32 time_ms);
// calls the DW1000 interrupt handler
#define instance_process_irq(x) 	dwt_isr()  //call device interrupt handler
// configure TX/RX callback functions that are called from DW1000 ISR
void instance_rxcallback(const dwt_callback_data_t *rxd);
void instance_txcallback(const dwt_callback_data_t *txd);

//resets the instance back to initial state
void instance_reset(void);

// Go to pause mode when "run" input parameter is 0, Exit pause mode when "run" input parameter is 1.
void instance_pause(int run) ;
// Return true(1) if instance is in the pause state or false(0) if not
int isinstancepaused(void) ;

// Set all timings according to user configuration.
// param  anc_resp_dly_ms  Anchor response delay in milliseconds
// param  tag_resp_dly_ms  Tag response delay in milliseconds
// param  blink_period_ms  Tag blink period in milliseconds
// param  poll_period_ms  Tag poll period in milliseconds
void instance_set_user_timings(int anc_resp_dly_ms, int tag_resp_dly_ms,
                               int blink_period_ms, int poll_period_ms);

// Pre-compute frame lengths, timeouts and delays needed in ranging process.
// /!\ This function assumes that there is no user payload in the frame.
void instance_init_timings(void);

// set the SPI rate
int instancesetspibitrate(int newRateKHz) ;                         // Set SPI rate
// set/get the instance roles e.g. Tag/Anchor/Listener
void instancesetrole(int mode) ;                // 
int instancegetrole(void) ;

// get the DW1000 device ID (e.g. 0xDECA0130 for MP)
uint32 instancereaddeviceid(void) ;                                 // Return Device ID reg, enables validation of physical device presence

int instancegetRxCount(void);

int instancegetRngCount(void);

#define DWT_PRF_64M_RFDLY   (514.462f)
#define DWT_PRF_16M_RFDLY   (513.9067f)
extern const uint16 rfDelays[2];
extern const tx_struct txSpectrumConfig[8];

int instance_starttxtest(int framePeriod);

int instance_startcwmode(int chan);

int instance_peekevent(void);

void instance_putevent(event_data_t newevent);

event_data_t instance_getevent(void);

void instance_clearevents(void);

void configureREKHW(int chan, int is628);

#ifdef __cplusplus
}
#endif

#endif
