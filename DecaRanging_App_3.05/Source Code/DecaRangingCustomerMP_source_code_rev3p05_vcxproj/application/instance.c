/*! ------------------------------------------------------------------------------------------------------------------
 * @file	instance.c
 * @brief	application level message exchange for ranging demo
 *
 * @attention
 * 
 * Copyright 2008, 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */

#include "lib.h"
#include "compiler.h"
#include "deca_device_api.h"
#include "deca_spinit.h"
#include "deca_spi.h"
#include "deca_util.h"
#include "instance.h"

// -------------------------------------------------------------------------------------------------------------------
//      Data Definitions
// -------------------------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------------------------
instance_data_t instance_data[NUM_INST] ;

// -------------------------------------------------------------------------------------------------------------------
// Functions
// -------------------------------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------------------------------
//
// function to construct the message/frame header bytes
//
// -------------------------------------------------------------------------------------------------------------------
//
void instanceconfigframeheader(instance_data_t *inst)
{
    inst->msg.panID[0] = (inst->panid) & 0xff;
    inst->msg.panID[1] = inst->panid >> 8;

    //set frame type (0-2), SEC (3), Pending (4), ACK (5), PanIDcomp(6)
    inst->msg.frameCtrl[0] = 0x1 /*frame type 0x1 == data*/ | 0x40 /*PID comp*/;
#if (USING_64BIT_ADDR==1)
    //source/dest addressing modes and frame version
    inst->msg.frameCtrl[1] = 0xC /*dest extended address (64bits)*/ | 0xC0 /*src extended address (64bits)*/;
#else
	inst->msg.frameCtrl[1] = 0x8 /*dest short address (16bits)*/ | 0x80 /*src short address (16bits)*/;
#endif
    inst->msg.seqNum = inst->frame_sn++;
}

// -------------------------------------------------------------------------------------------------------------------
//
// function to configure the frame data, prior to writing the frame to the TX buffer
//
// -------------------------------------------------------------------------------------------------------------------
//
void setupmacframedata(instance_data_t *inst, int fcode)
{
    inst->msg.messageData[FCODE] = fcode; //message function code (specifies if message is a poll, response or other...)
	instanceconfigframeheader(inst);
}

// -------------------------------------------------------------------------------------------------------------------
//
// Turn on the receiver with/without delay
//
void instancerxon(instance_data_t *inst, int delayed, uint64 delayedReceiveTime)
{
    if (delayed)
    {
    	uint32 dtime;
    	dtime =  (uint32) (delayedReceiveTime>>8);
        dwt_setdelayedtrxtime(dtime) ;
    }

    inst->lateRX -= dwt_rxenable(delayed) ;  //- as when fails -1 is returned             // turn receiver on, immediate/delayed

} // end instancerxon()


int instancesendpacket(uint16 length, uint8 txmode, uint32 dtime)
{
    int result = 0;

    dwt_writetxfctrl(length, 0);
    if (txmode & DWT_START_TX_DELAYED)
    {
        dwt_setdelayedtrxtime(dtime);
    }

    //begin delayed TX of frame
    if (dwt_starttx(txmode))  // delayed start was too late
    {
        result = 1; //late/error
    }

    return result;                                              // state changes

}

// -------------------------------------------------------------------------------------------------------------------
//
// the main instance state machine (all the instance modes Tag, Anchor or Listener use the same statemachine....)
//
// -------------------------------------------------------------------------------------------------------------------
//
int testapprun(instance_data_t *inst, int message, uint32 time_ms)
{

    switch (inst->testAppState)
    {
        case TA_INIT :
            // printf("TA_INIT") ;
            switch (inst->mode)
            {
				case TAG:
				case TAG_TDOA:
				{
					dwt_enableframefilter(DWT_FF_DATA_EN | DWT_FF_ACK_EN); //allow data, ack frames;
                    inst->frameFilteringEnabled = 1 ;
					dwt_setpanid(inst->panid);
					dwt_seteui(inst->eui64);
#if (USING_64BIT_ADDR==0)
					//the short address is assigned by the anchor
#else
					//set source address into the message structure
                    memcpy(&inst->msg.sourceAddr[0], inst->eui64, ADDR_BYTE_SIZE_L);
#endif

					//change to next state - send a Poll message to 1st anchor in the list
					inst->mode = TAG_TDOA ;
					inst->testAppState = TA_TXBLINK_WAIT_SEND;	
					memcpy(inst->blinkmsg.tagID, inst->eui64, ADDR_BYTE_SIZE_L);
				}
				break;
				case ANCHOR:
				{

					dwt_enableframefilter(DWT_FF_NOTYPE_EN); //disable frame filtering
                    inst->frameFilteringEnabled = 0 ;
					dwt_seteui(inst->eui64);
                    dwt_setpanid(inst->panid);

					
#if (USING_64BIT_ADDR==0)
					{
						uint16 addr = inst->eui64[0] + (inst->eui64[1] << 8);
						dwt_setaddress16(addr);
						//set source address into the message structure
						memcpy(&inst->msg.sourceAddr[0], inst->eui64, ADDR_BYTE_SIZE_S);
						//set source address into the message structure
						memcpy(&inst->rng_initmsg.sourceAddr[0], inst->eui64, ADDR_BYTE_SIZE_S);
					}
#else
					//set source address into the message structure
                    memcpy(&inst->msg.sourceAddr[0], inst->eui64, ADDR_BYTE_SIZE_L);
					//set source address into the message structure
					memcpy(&inst->rng_initmsg.sourceAddr[0], inst->eui64, ADDR_BYTE_SIZE_L);
#endif

					// First time anchor listens we don't do a delayed RX
					dwt_setrxaftertxdelay(0);
					//change to next state - wait to receive a message
					inst->testAppState = TA_RXE_WAIT ;

					dwt_setrxtimeout(0);

				}
				break;
				case LISTENER:
				{
                    dwt_enableframefilter(DWT_FF_NOTYPE_EN); //disable frame filtering
                    inst->frameFilteringEnabled = 0 ;
					// First time anchor listens we don't do a delayed RX
					dwt_setrxaftertxdelay(0);
					//change to next state - wait to receive a message
					inst->testAppState = TA_RXE_WAIT ;

					dwt_setrxtimeout(0);

					instsettagtorangewith(TAG_LIST_SIZE); //not associated any longer
				}
				break ; // end case TA_INIT
				default:
				break;
            }
            break; // end case TA_INIT

        
        case TA_PAUSED :    // (idle) state when application pause button is pressed

            switch (inst->pauseRequest)
            {
            case 0 : // exit pause
                // setup and return to Anchor/Tag/Listener mode of operation.
                instance_reset();
                break ;

            case 1 : // entering pause state
                dwt_forcetrxoff() ;         // put IC into idle mode
                inst->pauseRequest = 2 ;    // in paused state, so default case below will be done next time.
                break ;

            default:
                // stay paused while non-zero value
                break ;
            }

			inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;     // say we are "done" so we exit the instance_run while loop 

			break ; // end case TA_PAUSED


		case TA_SLEEP_DONE :
			{
				event_data_t dw_event = instance_getevent(); //clear the event from the queue
				// waiting for timout from application to wakup IC, or if pause is requested wakeup immediately
				if ((dw_event.type != DWT_SIG_RX_TIMEOUT) && (inst->pauseRequest == 0) )
				{
					// if no pause and no wake-up timeout continu waiting for the sleep to be done.
					inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //wait here for sleep timeout
					break;
				}
			
				inst->done = INST_NOT_DONE_YET;
				inst->testAppState = inst->nextState;
				inst->nextState = 0; //clear

				inst->instancetimer1 = time_ms;
			}
            break;

        case TA_TXE_WAIT : //either go to sleep or proceed to TX a message
            //printf("TA_TXE_WAIT") ;
			//if we are scheduled to go to sleep before next sending then sleep first.
			if(((inst->nextState == TA_TXPOLL_WAIT_SEND)
				|| (inst->nextState == TA_TXBLINK_WAIT_SEND))) 
            {
                //the app should put chip into low power state and wake up in tagSleepTime_ms time...
                //the app could go to *_IDLE state and wait for uP to wake it up...
				inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT_TO; //don't sleep here but kick off the TagTimeoutTimer (instancetimer)
				inst->testAppState = TA_SLEEP_DONE;
                // Now that we have time, update displayed range if needed.
                if (inst->tof > 0)
                {
                    reportTOF(inst);
                    // Clear TOF now that it has been reported.
                    inst->tof = 0;
                }
            }
            else //proceed to configuration and transmission of a frame
            {
				inst->testAppState = inst->nextState;
				inst->nextState = 0; //clear
            }
            break ; // end case TA_TXE_WAIT

        case TA_TXBLINK_WAIT_SEND :
            {
				int flength = (BLINK_FRAME_CRTL_AND_ADDRESS + FRAME_CRC);
                if (inst->pauseRequest) 
                {
                    inst->testAppState = TA_PAUSED ;
                    break ; // done with current state processing
                }

                //blink frames with IEEE EUI-64 tag ID
				inst->blinkmsg.frameCtrl = 0xC5 ;
                inst->blinkmsg.seqNum = inst->frame_sn++;

                dwt_writetxdata(flength, (uint8 *)  (&inst->blinkmsg), 0) ;	// write the frame data
				dwt_writetxfctrl(flength, 0);


				//using wait for response to do delayed receive
				inst->wait4ack = DWT_RESPONSE_EXPECTED;

				dwt_setrxtimeout((uint16)inst->fwtoTimeB_sy);  //units are symbols
				//set the delayed rx on time (the ranging init will be sent after this delay)
				dwt_setrxaftertxdelay((uint32)inst->rnginitW4Rdelay_sy);  //units are 1.0256us - wait for wait4respTIM before RX on (delay RX)

                dwt_starttx(DWT_START_TX_IMMEDIATE | inst->wait4ack); //always using immediate TX and enable dealyed RX

				inst->testAppState = TA_TX_WAIT_CONF ; // wait confirmation
                inst->previousState = TA_TXBLINK_WAIT_SEND ;
                inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //will use RX FWTO to time out (set below)

            }
            break ; // end case TA_TXBLINK_WAIT_SEND

		case TA_TXRANGINGINIT_WAIT_SEND :
		{
                uint16 resp_delay;

                inst->psduLength = RANGINGINIT_MSG_LEN;

                //tell Tag what it's address will be for the ranging exchange
                inst->rng_initmsg.messageData[FCODE] = RTLS_DEMO_MSG_RNG_INIT;
                inst->rng_initmsg.messageData[RNG_INIT_TAG_SHORT_ADDR_LO] = inst->tagShortAdd & 0xFF;
                inst->rng_initmsg.messageData[RNG_INIT_TAG_SHORT_ADDR_HI] = (inst->tagShortAdd >> 8) & 0xFF;

                // First response delay to send is anchor's response delay.
                resp_delay = ((RESP_DLY_UNIT_MS << RESP_DLY_UNIT_SHIFT) & RESP_DLY_UNIT_MASK)
                    + ((inst->anc_resp_dly_ms << RESP_DLY_VAL_SHIFT) & RESP_DLY_VAL_MASK);
                inst->rng_initmsg.messageData[RNG_INIT_ANC_RESP_DLY_LO] = resp_delay & 0xFF;
                inst->rng_initmsg.messageData[RNG_INIT_ANC_RESP_DLY_HI] = (resp_delay >> 8) & 0xFF;
                // Second response delay to send is tag's response delay.
                resp_delay = ((RESP_DLY_UNIT_MS << RESP_DLY_UNIT_SHIFT) & RESP_DLY_UNIT_MASK)
                    + ((inst->tag_resp_dly_ms << RESP_DLY_VAL_SHIFT) & RESP_DLY_VAL_MASK);
                inst->rng_initmsg.messageData[RNG_INIT_TAG_RESP_DLY_LO] = resp_delay & 0xFF;
                inst->rng_initmsg.messageData[RNG_INIT_TAG_RESP_DLY_HI] = (resp_delay >> 8) & 0xFF;

                inst->rng_initmsg.frameCtrl[0] = 0x41; //

#if (USING_64BIT_ADDR == 1)				
				inst->rng_initmsg.frameCtrl[1] = 0xCC;
				inst->psduLength += FRAME_CRTL_AND_ADDRESS_L + FRAME_CRC;
#else
				inst->rng_initmsg.frameCtrl[1] = 0x8C;
				inst->psduLength += FRAME_CRTL_AND_ADDRESS_LS + FRAME_CRC;
#endif
				inst->rng_initmsg.panID[0] = (inst->panid) & 0xff;
				inst->rng_initmsg.panID[1] = inst->panid >> 8;

				inst->rng_initmsg.seqNum = inst->frame_sn++;

				inst->wait4ack = DWT_RESPONSE_EXPECTED;

                inst->testAppState = TA_TX_WAIT_CONF;                                               // wait confirmation
                inst->previousState = TA_TXRANGINGINIT_WAIT_SEND ;
                
				dwt_writetxdata(inst->psduLength, (uint8 *)  &inst->rng_initmsg, 0) ;	// write the frame data

				//anchor - we don't use timeout, if the ACK is missed we'll get a Poll or Blink

                if (instancesendpacket(inst->psduLength, DWT_START_TX_DELAYED | DWT_RESPONSE_EXPECTED, inst->delayedReplyTime))
                {
                    dwt_setrxaftertxdelay(0);
                    inst->testAppState = TA_RXE_WAIT;  // wait to receive a new blink or poll message
                    inst->wait4ack = 0; //clear the flag as the TX has failed the TRX is off
                    inst->lateTX++;

                }
                else
                {
                    inst->testAppState = TA_TX_WAIT_CONF;                                               // wait confirmation
                    inst->previousState = TA_TXRANGINGINIT_WAIT_SEND;
                    inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;  //no timeout

                    //CONFIGURE FIXED PARTS OF RESPONSE MESSAGE FRAME (these won't change)
                    //program option octet and parameters (not used currently)
                    inst->msg.messageData[RES_R1] = 0x2; // "activity"
                    inst->msg.messageData[RES_R2] = 0x0; //
                    inst->msg.messageData[RES_R3] = 0x0;
                    //set the destination address in the TWR response message
#if 0
#if (USING_64BIT_ADDR == 1)
                    memcpy(&inst->msg.destAddr[0], &inst->rng_initmsg.destAddr[0], ADDR_BYTE_SIZE_L); //remember who to send the reply to (set destination address)
#else
                    memcpy(&inst->msg.destAddr[0], &inst->tagShortAdd, ADDR_BYTE_SIZE_S); //remember who to send the reply to (set destination address)
#endif
#endif

                    setupmacframedata(inst, RTLS_DEMO_MSG_ANCH_RESP);
                    //inst->timeofTx = portGetTickCnt();
                    //inst->monitor = 1;
                }

            }
            break;

        case TA_TXPOLL_WAIT_SEND :
            {
				//printf("TA_TXPOLL_WAIT_SEND @%d\n", time_ms) ;
                if (inst->pauseRequest) 
                {
                    inst->testAppState = TA_PAUSED ;
                    break ; // done with current state processing
                }

				//NOTE the anchor address is set after receiving the ranging initialisation message

                inst->msg.seqNum = inst->frame_sn++;
                setupmacframedata(inst, RTLS_DEMO_MSG_TAG_POLL);
#if (USING_64BIT_ADDR==1)

                inst->psduLength = TAG_POLL_MSG_LEN + FRAME_CRTL_AND_ADDRESS_L + FRAME_CRC;
#else
                inst->psduLength = TAG_POLL_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC;
#endif
				//set the delayed rx on time (the response message will be sent after this delay)
                dwt_setrxaftertxdelay(inst->txToRxDelayTag_sy);
                dwt_setrxtimeout((uint16)inst->fwtoTime_sy);
				
				dwt_writetxdata(inst->psduLength, (uint8 *)  &inst->msg, 0) ;	// write the frame data
				
				//response is expected
				inst->wait4ack = DWT_RESPONSE_EXPECTED;
				
				dwt_writetxfctrl(inst->psduLength, 0);
				dwt_starttx(DWT_START_TX_IMMEDIATE | inst->wait4ack);

                inst->testAppState = TA_TX_WAIT_CONF ;                                               // wait confirmation
                inst->previousState = TA_TXPOLL_WAIT_SEND ;
                inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //will use RX FWTO to time out (set below)

            }
            break;

        case TA_TXRESPONSE_WAIT_SEND : //the frame is loaded and sent from the RX callback
		    {
               //printf("TA_TXRESPONSE\n") ;
			    if (inst->pauseRequest) 
                {
                    inst->testAppState = TA_PAUSED ;
                    break ; // done with current state processing
                }

                inst->testAppState = TA_TX_WAIT_CONF;                                               // wait confirmation
                inst->previousState = TA_TXRESPONSE_WAIT_SEND ;
            }
            break;

        case TA_TXFINAL_WAIT_SEND :
            {
                if (inst->pauseRequest) 
                {
                    inst->testAppState = TA_PAUSED ;
                    break ; // done with current state processing
                }

                // Embbed into Final message: 40-bit respRxTime
                // Write Response RX time field of Final message
                memcpy(&(inst->msg.messageData[RRXT]), (uint8 *)&inst->anchorRespRxTime, 5);

                setupmacframedata(inst, RTLS_DEMO_MSG_TAG_FINAL);
#if (USING_64BIT_ADDR==1)
                inst->psduLength = TAG_FINAL_MSG_LEN + FRAME_CRTL_AND_ADDRESS_L + FRAME_CRC;
#else
                inst->psduLength = TAG_FINAL_MSG_LEN + FRAME_CRTL_AND_ADDRESS_S + FRAME_CRC;
#endif
				dwt_writetxdata(inst->psduLength, (uint8 *)  &inst->msg, 0) ;	// write the frame data

                if (instancesendpacket(inst->psduLength, DWT_START_TX_DELAYED, inst->delayedReplyTime))
                {
                    // initiate the re-transmission
                    inst->testAppState = TA_TXE_WAIT ;  
                    inst->nextState = TA_TXPOLL_WAIT_SEND ;
					inst->wait4ack = 0; //clear the flag as the TX has failed the TRX is off 
                    inst->lateTX++;

                    break; //exit this switch case...
                }
                else
                {
                    inst->testAppState = TA_TX_WAIT_CONF;                                               // wait confirmation
                    inst->previousState = TA_TXFINAL_WAIT_SEND;
                	inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //will use RX FWTO to time out  (set below)
					//inst->responseTimeouts = 0; //reset response timeout count
                    inst->timeofTx = time_ms;
                    inst->monitor = 1;
                }
            }
            break;

        case TA_TX_WAIT_CONF :
           //printf("TA_TX_WAIT_CONF p%d m%d states %08x %08x\n", inst->previousState, message, dwt_read32bitreg(0x19), dwt_read32bitreg(0x0f)) ;
			
			{
				event_data_t dw_event = instance_getevent(); //get and clear this event

				//NOTE: Can get the ACK before the TX confirm event for the frame requesting the ACK
				//this happens because if polling the ISR the RX event will be processed 1st and then the TX event
				//thus the reception of the ACK will be processed before the TX confirmation of the frame that requested it.
            	if(dw_event.type != DWT_SIG_TX_DONE) //wait for TX done confirmation  
            	{
					if(dw_event.type == DWT_SIG_RX_TIMEOUT) //got RX timeout - i.e. did not get the response (e.g. ACK)
    	        	{
						printf("RX timeout in TA_TX_WAIT_CONF (%d) @%d\n", inst->previousState, time_ms);
            	        //we need to wait for SIG_TX_DONE and then process the timeout and re-send the frame if needed
                	    inst->gotTO = 1;
            		}
					if(dw_event.type == DWT_SIG_RX_OKAY) //because ISR processes RX event before TX we can have a response message before the TX confirm has been processed
					{
						//inst->wait4ack = 0 ; //clear the flag as we got RX event
						//queue up the event again (to process after TX confirm)
						instance_putevent(dw_event);
					}

					inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;
            		break;

				}

				inst->done = INST_NOT_DONE_YET;

				if(inst->previousState == TA_TXFINAL_WAIT_SEND)
				{
					inst->testAppState = TA_TXE_WAIT ;                      
					inst->nextState = TA_TXPOLL_WAIT_SEND ;
					break;
				}
                else if (inst->gotTO) //timeout
                {
					//printf("got TO in TA_TX_WAIT_CONF\n");
                    inst_processrxtimeout(inst);
                    inst->gotTO = 0;
					inst->wait4ack = 0 ; //clear this
					break;
                }
				else
				{
					inst->txu.txTimeStamp = dw_event.timeStamp;
                    
                    if (inst->previousState == TA_TXPOLL_WAIT_SEND)
                    {
                        uint64 tagCalculatedFinalTxTime;
                        // Embed into Final message: 40-bit pollTXTime,  40-bit respRxTime,  40-bit finalTxTime

                        tagCalculatedFinalTxTime = (inst->txu.txTimeStamp + inst->finalReplyDelay) & MASK_TXDTS;  // time we should send the response
                        inst->delayedReplyTime = (uint32) (tagCalculatedFinalTxTime >> 8);

                        // Calculate Time Final message will be sent and write this field of Final message
                        // Sending time will be delayedReplyTime, snapped to ~125MHz or ~250MHz boundary by
                        // zeroing its low 9 bits, and then having the TX antenna delay added
                        // getting antenna delay from the device and add it to the Calculated TX Time
                        tagCalculatedFinalTxTime = tagCalculatedFinalTxTime + inst->txantennaDelay;
                        tagCalculatedFinalTxTime &= MASK_40BIT;

                        // Write Calculated TX time field of Final message
                        memcpy(&(inst->msg.messageData[FTXT]), (uint8 *)&tagCalculatedFinalTxTime, 5);
                        // Write Poll TX time field of Final message
                        memcpy(&(inst->msg.messageData[PTXT]), (uint8 *)&inst->txu.tagPollTxTime, 5);
                    }

					inst->testAppState = TA_RXE_WAIT ;                      // After sending, tag expects response/report, anchor waits to receive a final/new poll
					//fall into the next case (turn on the RX)
					message = 0;
				}

            }

            //break ; // end case TA_TX_WAIT_CONF


        case TA_RXE_WAIT :
        //printf("TA_RXE_WAIT %d\n", inst->wait4ack) ;
        {

            if(inst->wait4ack == 0) //if this is set the RX will turn on automatically after TX
            {
				//turn RX on
				instancerxon(inst, 0, 0) ;   // turn RX on, with/without delay
            }
			else
			{
				inst->wait4ack = 0 ; //clear the flag, the next time we want to turn the RX on it might not be auto
			}

			if (inst->mode != LISTENER)
			{
                //we are going to use anchor/tag timeout
				inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT; //using RX FWTO
			}

        	inst->testAppState = TA_RX_WAIT_DATA;   // let this state handle it

        	// end case TA_RXE_WAIT, don't break, but fall through into the TA_RX_WAIT_DATA state to process it immediately.
			if(message == 0) break;
		}

        case TA_RX_WAIT_DATA :                                                                     // Wait RX data
           //if(message) printf("TA_RX_WAIT_DATA %d\n", message) ;

            if (inst->pauseRequest) 
            {
                inst->testAppState = TA_PAUSED ;
                break ; // done with current state processing
            }

            switch (message)
            {
				case SIG_RX_BLINK : 
				{
					event_data_t dw_event = instance_getevent(); //get and clear this event
					//printf("we got blink message from %08X\n", ( tagaddr& 0xFFFF));
					if((inst->mode == LISTENER) || (inst->mode == ANCHOR))
					{
						//add this Tag to the list of Tags we know about
						instaddtagtolist(inst, &(dw_event.msgu.rxblinkmsg.tagID[0]));

						//initiate ranging message
						if(inst->tagToRangeWith < TAG_LIST_SIZE)
						{
							//initiate ranging message this is a Blink from the Tag we would like to range to
                        	if(memcmp(&inst->tagList[inst->tagToRangeWith][0],  &(dw_event.msgu.rxblinkmsg.tagID[0]), BLINK_FRAME_SOURCE_ADDRESS) == 0)
							{
								inst->tagShortAdd = (dwt_getpartid() & 0xFF); 
								inst->tagShortAdd =  (inst->tagShortAdd << 8) + dw_event.msgu.rxblinkmsg.tagID[0] ;

                                //if using longer reply delay time (e.g. if interworking with a PC application)
                                inst->delayedReplyTime = (uint32) ((dw_event.timeStamp + inst->rnginitReplyDelay) >> 8);  // time we should send the blink response

								//set destination address
                                memcpy(&inst->rng_initmsg.destAddr[0], &(dw_event.msgu.rxblinkmsg.tagID[0]), BLINK_FRAME_SOURCE_ADDRESS); //remember who to send the reply to

								inst->testAppState = TA_TXE_WAIT;
								inst->nextState = TA_TXRANGINGINIT_WAIT_SEND ; 

								break;
							}

							//else stay in RX
						}
					}
					//else //not initiating ranging - continue to receive 
					{
                        inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
						inst->done = INST_NOT_DONE_YET;
					}

				}
				break;

				//if we have received a DWT_SIG_RX_OKAY event - this means that the message is IEEE data type - need to check frame control to know which addressing mode is used
                case DWT_SIG_RX_OKAY :
                {
					event_data_t dw_event = instance_getevent(); //get and clear this event
					uint8  srcAddr[8] = {0,0,0,0,0,0,0,0};
					int uplen = 0;
					int fcode = 0;
					int fn_code = 0;
					//int srclen = 0;
					//int fctrladdr_len;
					uint8 *messageData;

                    inst->stoptimer = 0; //clear the flag, as we have received a message 
					
					// 16 or 64 bit addresses
					switch(dw_event.msgu.frame[1])
					{	
						case 0xCC: //
							memcpy(&srcAddr[0], &(dw_event.msgu.rxmsg_ll.sourceAddr[0]), ADDR_BYTE_SIZE_L);
							fn_code = dw_event.msgu.rxmsg_ll.messageData[FCODE];
							messageData = &dw_event.msgu.rxmsg_ll.messageData[0];
							//srclen = ADDR_BYTE_SIZE_L;
							//fctrladdr_len = FRAME_CRTL_AND_ADDRESS_L;
							break;
						case 0xC8: //
							memcpy(&srcAddr[0], &(dw_event.msgu.rxmsg_sl.sourceAddr[0]), ADDR_BYTE_SIZE_L);
							fn_code = dw_event.msgu.rxmsg_sl.messageData[FCODE];
							messageData = &dw_event.msgu.rxmsg_sl.messageData[0];
							//srclen = ADDR_BYTE_SIZE_L;
                            //fctrladdr_len = FRAME_CRTL_AND_ADDRESS_LS;
							break;
						case 0x8C: //
							memcpy(&srcAddr[0], &(dw_event.msgu.rxmsg_ls.sourceAddr[0]), ADDR_BYTE_SIZE_S);
							fn_code = dw_event.msgu.rxmsg_ls.messageData[FCODE];
							messageData = &dw_event.msgu.rxmsg_ls.messageData[0];
                            //srclen = ADDR_BYTE_SIZE_S;
                            //fctrladdr_len = FRAME_CRTL_AND_ADDRESS_LS;
							break;
						case 0x88: //
							memcpy(&srcAddr[0], &(dw_event.msgu.rxmsg_ss.sourceAddr[0]), ADDR_BYTE_SIZE_S);
							fn_code = dw_event.msgu.rxmsg_ss.messageData[FCODE];
							messageData = &dw_event.msgu.rxmsg_ss.messageData[0];
                            //srclen = ADDR_BYTE_SIZE_S;
                            //fctrladdr_len = FRAME_CRTL_AND_ADDRESS_S;
							break;
					}

                    {

						if(inst->mode == ANCHOR)
						{
#if (USING_64BIT_ADDR==1)
                        	if(memcmp(&inst->tagList[inst->tagToRangeWith][0], &srcAddr[0], BLINK_FRAME_SOURCE_ADDRESS) == 0) //if the Tag's address does not match (ignore the message)
#else
							//if using 16-bit addresses the ranging messages from tag are using the short address tag was given in the ranging init message
							if(inst->tagShortAdd == (srcAddr[0] + (srcAddr[1] << 8)))
#endif
							//only process messages from the associated tag
							{
								fcode = fn_code;
							}
						}
						else // LISTENER or TAG
						{
							fcode = fn_code;
						}

                        switch(fcode)
                        {
							case RTLS_DEMO_MSG_RNG_INIT:
                            {
								if(inst->mode == TAG_TDOA) //only start ranging with someone if not ranging already
								{
                                    uint32 final_reply_delay_us;
                                    uint32 resp_dly[RESP_DLY_NB];
                                    int i;

                                    inst->testAppState = TA_TXE_WAIT;
                                    inst->nextState = TA_TXPOLL_WAIT_SEND; // send next poll

                                    inst->tagShortAdd = messageData[RNG_INIT_TAG_SHORT_ADDR_LO]
                                        + (messageData[RNG_INIT_TAG_SHORT_ADDR_HI] << 8);

                                    // Get response delays from message and update internal timings accordingly
                                    resp_dly[RESP_DLY_ANC] = messageData[RNG_INIT_ANC_RESP_DLY_LO]
                                        + (messageData[RNG_INIT_ANC_RESP_DLY_HI] << 8);
                                    resp_dly[RESP_DLY_TAG] = messageData[RNG_INIT_TAG_RESP_DLY_LO]
                                        + (messageData[RNG_INIT_TAG_RESP_DLY_HI] << 8);
                                    for (i = 0; i < RESP_DLY_NB; i++)
                                    {
                                        if (((resp_dly[i] & RESP_DLY_UNIT_MASK) >> RESP_DLY_UNIT_SHIFT) == RESP_DLY_UNIT_MS)
                                        {
                                            // Remove unit bit and convert to microseconds.
                                            resp_dly[i] &= ~RESP_DLY_UNIT_MASK;
                                            resp_dly[i] *= 1000;
                                        }
                                    }
                                    // Update delay between poll transmission and response reception.
                                    // Use uint64 for resp_dly here to avoid overflows if it is more than 400 ms.
                                    inst->txToRxDelayTag_sy = US_TO_SY_INT((uint64)resp_dly[RESP_DLY_ANC] - inst->fl_us[POLL]) - RX_START_UP_SY;
                                    // Update delay between poll transmission and final transmission.
                                    final_reply_delay_us = resp_dly[RESP_DLY_ANC] + resp_dly[RESP_DLY_TAG];
                                    inst->finalReplyDelay = convertmicrosectodevicetimeu(final_reply_delay_us);
                                    inst->finalReplyDelay_ms = CEIL_DIV(final_reply_delay_us, 1000);

#if (USING_64BIT_ADDR == 1)
									memcpy(&inst->msg.destAddr[0], &srcAddr[0], ADDR_BYTE_SIZE_L); //set the anchor address for the reply (set destination address)
#else
									memcpy(&inst->msg.destAddr[0], &srcAddr[0], ADDR_BYTE_SIZE_S); //set anchor address for the reply (set destination address)
									inst->msg.sourceAddr[0] =  messageData[RES_R1]; //set tag short address
									inst->msg.sourceAddr[1] =  messageData[RES_R2];
									dwt_setaddress16(inst->tagShortAdd);
#endif
    
									inst->mode = TAG ;
									//inst->responseTimeouts = 0; //reset timeout count
                                    inst->instancetimer1 = time_ms; //reset the timer base
								}
                                //printf("GOT RTLS_DEMO_MSG_RNG_INIT - start ranging - \n");
								//else we ignore this message if already associated... (not TAG_TDOA)
                            }
                            break; //RTLS_DEMO_MSG_RNG_INIT

                            case RTLS_DEMO_MSG_TAG_POLL:
                            {
								if(inst->mode == LISTENER) //don't process any ranging messages when in Listener mode
								{
                        			inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
								    break;
								}

                                if (!inst->frameFilteringEnabled)
                                {
                                    // if we missed the ACK to the ranging init message we may not have turned frame filtering on
                                    dwt_enableframefilter(DWT_FF_DATA_EN | DWT_FF_ACK_EN); //we are starting ranging - enable the filter....
                                    inst->frameFilteringEnabled = 1 ;
                                }

                                if (dw_event.type3 == DWT_SIG_TX_PENDING)
                                {
                                    inst->testAppState = TA_TX_WAIT_CONF;                                               // wait confirmation
                                    inst->previousState = TA_TXRESPONSE_WAIT_SEND;
                                }
                                else
                                {
                                    //stay in RX wait for next frame...
                                    inst->testAppState = TA_RXE_WAIT;              // wait for next frame
                                }
                            }
                            break; //RTLS_DEMO_MSG_TAG_POLL

                            case RTLS_DEMO_MSG_ANCH_RESP:
                            {
								if(inst->mode == LISTENER) //don't process any ranging messages when in Listener mode
								{
                        			inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
									break;
								}
								
                                inst->anchorRespRxTime = dw_event.timeStamp ; //Response's Rx time

                                inst->testAppState = TA_TXFINAL_WAIT_SEND ; // send our response / the final

                                inst->tof = 0;
                                //copy previously calculated ToF
                                memcpy(&inst->tof, &(messageData[TOFR]), 5);
                            }
                            break; //RTLS_DEMO_MSG_ANCH_RESP

                            case RTLS_DEMO_MSG_TAG_FINAL:
                            {
                                int64 Rb, Da, Ra, Db;
                                uint64 tagFinalTxTime = 0;
                                uint64 tagFinalRxTime = 0;
                                uint64 tagPollTxTime = 0;
                                uint64 anchorRespRxTime = 0;

                                double RaRbxDaDb = 0;
                                double RbyDb = 0;
                                double RayDa = 0;

								if(inst->mode == LISTENER) //don't process any ranging messages when in Listener mode
								{
                        			inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
									break;
								}

                                // time of arrival of Final message
                                tagFinalRxTime = dw_event.timeStamp ; //Final's Rx time

								inst->delayedReplyTime = 0 ;

                                // times measured at Tag extracted from the message buffer
								// extract 40bit times 
                                memcpy(&tagPollTxTime, &(messageData[PTXT]), 5);
                                memcpy(&anchorRespRxTime, &(messageData[RRXT]), 5);
                                memcpy(&tagFinalTxTime, &(messageData[FTXT]), 5);

                                // poll response round trip delay time is calculated as
                                // (anchorRespRxTime - tagPollTxTime) - (anchorRespTxTime - tagPollRxTime)
                                Ra = (int64)((anchorRespRxTime - tagPollTxTime) & MASK_40BIT);
                                Db = (int64)((inst->txu.anchorRespTxTime - inst->tagPollRxTime) & MASK_40BIT);

                                // response final round trip delay time is calculated as
                                // (tagFinalRxTime - anchorRespTxTime) - (tagFinalTxTime - anchorRespRxTime)
                                Rb = (int64)((tagFinalRxTime - inst->txu.anchorRespTxTime) & MASK_40BIT);
                                Da = (int64)((tagFinalTxTime - anchorRespRxTime) & MASK_40BIT);

                                RaRbxDaDb = (((double)Ra))*(((double)Rb))
                                    - (((double)Da))*(((double)Db));

                                RbyDb = ((double)Rb + (double)Db);

                                RayDa = ((double)Ra + (double)Da);

                                //time-of-flight
                                inst->tof = (int64)(RaRbxDaDb / (RbyDb + RayDa));

                                // Compute clock offset, in parts per million
                                inst->clockOffset = ((((double)Ra / 2) - (double)inst->tof) / (double)Db)
                                                    - ((((double)Rb / 2) - (double)inst->tof) / (double)Da);
                                inst->clockOffset *= 1e6;

								reportTOF(inst);
                                inst->lastReportTime = time_ms;

#if 1 //debug
#if (DECA_SUPPORT_SOUNDING==1)
	#if DECA_ACCUM_LOG_SUPPORT==1
		if ((inst->dwlogdata.accumLogging == LOG_ALL_ACCUM ) || (inst->dwlogdata.accumLogging == LOG_ALL_NOACCUM ))
		{
			double a1 = convertdevicetimetosecu(Ra)*1e9 - 150e6;
			double a2 = convertdevicetimetosecu(Db)*1e9 - 150e6;
			double b1 = convertdevicetimetosecu(Rb)*1e9 - 150e6;
			double b2 = convertdevicetimetosecu(Da)*1e9 - 150e6;

			fprintf(inst->dwlogdata.accumLogFile,"\nRa  %08x%08x Db %08x%08x\n", (uint32) (Ra >>32), (uint32) Ra, (uint32) (Db >>32), (uint32) Db) ;
			fprintf(inst->dwlogdata.accumLogFile,"\nRa1  %f Db1 %f  ns\n", a1, a2) ;
			fprintf(inst->dwlogdata.accumLogFile,"\nRb  %08x%08x Da %08x%08x\n", (uint32) (Rb >>32), (uint32) Rb, (uint32) (Da >>32), (uint32) Da) ;
			fprintf(inst->dwlogdata.accumLogFile,"\nRb1  %f Da1 %f  ns\n", b1, b2) ;			
		}
    #endif
#endif
#endif

                                inst->testAppState = TA_RXE_WAIT;              // wait for next frame
                                dwt_setrxaftertxdelay(0);
								
                            }
                            break; //RTLS_DEMO_MSG_TAG_FINAL


                            default:
                            {
                        		inst->testAppState = TA_RXE_WAIT ;              // wait for next frame
							    dwt_setrxaftertxdelay(0);

                            }
                            break;
						} //end switch (fcode)

						if(dw_event.msgu.frame[0] & 0x20)
						{
							//as we only pass the received frame with the ACK request bit set after the ACK has been sent
							instance_getevent(); //get and clear the ACK sent event
						}
					} //end else

					if((inst->mode == LISTENER) /*|| (inst->mode == ANCHOR)*/)//update received data, and go back to receiving frames
                    {

                        inst->testAppState = TA_RXE_WAIT ;              // wait for next frame

                        dwt_setrxaftertxdelay(0);
                    } 
			    }
				break ; //end of DWT_SIG_RX_OKAY

                case DWT_SIG_RX_TIMEOUT :
					{
						event_data_t dw_event = instance_getevent(); //get and clear this event
						//printf("PD_DATA_TIMEOUT %d\n", inst->previousState) ;
						inst_processrxtimeout(inst);
						message = 0; //clear the message as we have processed the event
						if(dw_event.type2 == (0x80 | DWT_SIG_RX_TIMEOUT)) //need to turn off transceiver when PC/application timer is used
							dwt_forcetrxoff();
					}
                break ;

				case DWT_SIG_TX_AA_DONE: //ignore this event - just process the rx frame that was received before the ACK response
                case 0:
                {
					//if(DWT_SIG_TX_AA_DONE == message) printf("Got SIG_TX_AA_DONE in RX wait - ignore\n");
                	if(inst->done == INST_NOT_DONE_YET) inst->done = INST_DONE_WAIT_FOR_NEXT_EVENT;

                }
                break;

                default :
                {
					printf("\nERROR - Unexpected message %d ??\n", message) ;
                    //assert(0) ;  
                }
                break ;

            }
            break ; // end case TA_RX_WAIT_DATA

            default:
                printf("\nERROR - invalid state %d - what is going on??\n", inst->testAppState) ;
            break;
    } // end switch on testAppState

    return inst->done;
} // end testapprun()

// -------------------------------------------------------------------------------------------------------------------
#if NUM_INST != 1
#error These functions assume one instance only
#else

// Set all timings according to user configuration.
// param  anc_resp_dly_ms  Anchor response delay in milliseconds
// param  tag_resp_dly_ms  Tag response delay in milliseconds
// param  blink_period_ms  Tag blink period in milliseconds
// param  poll_period_ms  Tag poll period in milliseconds
void instance_set_user_timings(int anc_resp_dly_ms, int tag_resp_dly_ms,
                               int blink_period_ms, int poll_period_ms)
{
    instance_data_t *inst = &instance_data[0];
    inst->anc_resp_dly_ms = anc_resp_dly_ms;
    inst->tag_resp_dly_ms = tag_resp_dly_ms;
    inst->blink_period_ms = blink_period_ms;
    inst->poll_period_ms = poll_period_ms;
}

extern uint8 dwnsSFDlen[];

// Pre-compute frame lengths, timeouts and delays needed in ranging process.
// /!\ This function assumes that there is no user payload in the frame.
void instance_init_timings(void)
{
    instance_data_t *inst = &instance_data[0];
    uint32 pre_len;
    int sfd_len;
    static const int data_len_bytes[FRAME_TYPE_NB] = {
        BLINK_FRAME_LEN_BYTES, RNG_INIT_FRAME_LEN_BYTES, POLL_FRAME_LEN_BYTES,
        RESP_FRAME_LEN_BYTES, FINAL_FRAME_LEN_BYTES };
    int i;
    // Margin used for timeouts computation.
    const int margin_sy = 10;

    // All internal computations are done in tens of picoseconds before
    // conversion into microseconds in order to ensure that we keep the needed
    // precision while not having to use 64 bits variables.

    // Compute frame lengths.
    // First step is preamble plus SFD length.
    sfd_len = dwnsSFDlen[inst->configData.dataRate];
    switch (inst->configData.txPreambLength)
    {
    case DWT_PLEN_4096:
        pre_len = 4096;
        break;
    case DWT_PLEN_2048:
        pre_len = 2048;
        break;
    case DWT_PLEN_1536:
        pre_len = 1536;
        break;
    case DWT_PLEN_1024:
        pre_len = 1024;
        break;
    case DWT_PLEN_512:
        pre_len = 512;
        break;
    case DWT_PLEN_256:
        pre_len = 256;
        break;
    case DWT_PLEN_128:
        pre_len = 128;
        break;
    case DWT_PLEN_64:
        pre_len = 64;
        break;
    }
    pre_len += sfd_len;
    // Convert preamble length from symbols to time. Length of symbol is defined
    // in IEEE 802.15.4 standard.
    if (inst->configData.prf == DWT_PRF_16M)
        pre_len *= 99359;
    else
        pre_len *= 101763;
    // Second step is data length for all frame types.
    for (i = 0; i < FRAME_TYPE_NB; i++)
    {
        // Compute the number of symbols for the given length.
        inst->fl_us[i] = data_len_bytes[i] * 8
            + CEIL_DIV(data_len_bytes[i], 330) * 48;
        // Convert from symbols to time and add PHY header length.
        if (inst->configData.dataRate == DWT_BR_110K)
        {
            inst->fl_us[i] *= 820513;
            inst->fl_us[i] += 17230800;
        }
        else if (inst->configData.dataRate == DWT_BR_850K)
        {
            inst->fl_us[i] *= 102564;
            inst->fl_us[i] += 2153900;
        }
        else
        {
            inst->fl_us[i] *= 12821;
            inst->fl_us[i] += 2153900;
        }
        // Last step: add preamble length and convert to microseconds.
        inst->fl_us[i] += pre_len;
        inst->fl_us[i] = CEIL_DIV(inst->fl_us[i], 100000);
    }
    // Final frame wait timeout time.
    inst->fwtoTime_sy = US_TO_SY_INT(inst->fl_us[FINAL])
        + RX_START_UP_SY + margin_sy;
    // Ranging init frame wait timeout time.
    inst->fwtoTimeB_sy = US_TO_SY_INT(inst->fl_us[RNG_INIT])
        + RX_START_UP_SY + margin_sy;
    // Delay between blink transmission and ranging init reception.
    inst->rnginitW4Rdelay_sy =
        US_TO_SY_INT((RNG_INIT_REPLY_DLY_MS * 1000) - inst->fl_us[BLINK])
        - RX_START_UP_SY;
    // Delay between anchor's response transmission and final reception.
    inst->txToRxDelayAnc_sy = US_TO_SY_INT(inst->tag_resp_dly_ms * 1000 - inst->fl_us[RESP])
                              - RX_START_UP_SY;

    // No need to init txToRxDelayTag_sy here as it will be set upon reception
    // of ranging init message.

    // Delay between blink reception and ranging init message transmission.
    inst->rnginitReplyDelay = convertmicrosectodevicetimeu(RNG_INIT_REPLY_DLY_MS * 1000);
    // Delay between poll reception and response transmission.
    inst->responseReplyDelay = convertmicrosectodevicetimeu(inst->anc_resp_dly_ms * 1000);
    inst->responseReplyDelay_ms = inst->anc_resp_dly_ms;

    // Smart Power is automatically applied by DW chip for frame of which length
    // is < 1 ms. Let the application know if it will be used depending on the
    // length of the longest frame.
    if (inst->fl_us[FINAL] <= 1000)
        inst->configData.smartPowerEn = 1;
    else
        inst->configData.smartPowerEn = 0;
}


// -------------------------------------------------------------------------------------------------------------------
//
// Access for low level debug
//
// -------------------------------------------------------------------------------------------------------------------


int instance_readaccumulatordata(void)
{
#if DECA_SUPPORT_SOUNDING==1
	int instance = 0;
    uint16 len = 992 ; //default (16M prf)

	if (instance_data[instance].configData.prf == DWT_PRF_64M)  // Figure out length to read
		len = 1016 ;       

    instance_data[instance].dwacclogdata.buff.accumLength = len ;  // remember Length, then read the accumulator data

    len = len*4+1 ;   // extra 1 as first byte is dummy due to internal memory access delay

	dwt_readaccdata((uint8*)&(instance_data[instance].dwacclogdata.buff.accumData->dummy), len, 0);

	//read the preamble data (from index 4096, 1024 bytes) - ignore 1st byte
    //dwt_readaccdata((uint8*)(instance_data[instance].dwacclogdata.preambData), 1024, 4097) ;
#endif  // support_sounding
    return 0;
}

// Set SPI rate

int instancesetspibitrate(int newRateKHz)
{
    int instance = 0 ;
    return setspibitrate(newRateKHz) ;
}

// -------------------------------------------------------------------------------------------------------------------

// Go to pause mode when "run" input parameter is 0, Exit pause mode when "run" input parameter is 1.
void instance_pause(int run)
{
	int instance = 0 ;
    int pause = 0 ;
    if (!run) pause = 1 ;
	instance_data[instance].pauseRequest = pause ;
}

// Return true(1) if instance is in the pause state or false(0) if not
int isinstancepaused(void)
{
	int instance = 0 ;
	if (instance_data[instance].testAppState == TA_PAUSED) return 1 ;
    return 0 ;
}

void instance_reset(void)
{
	//rest state back to idle (i.e. for Tag - send a Poll, and for Anchor back to Rx wait for data)
	int instance = 0 ;

	instance_clearevents();

	//disable the radio (if using SW timeout the rx will not be off)
	dwt_forcetrxoff() ;

	if(instance_data[instance].mode == TAG)
	{
		instance_data[instance].testAppState = TA_TXPOLL_WAIT_SEND;
	}
	else if(instance_data[instance].mode == TAG_TDOA)
	{
		instance_data[instance].testAppState = TA_TXBLINK_WAIT_SEND;
	}
	else
	{
		instance_data[instance].testAppState = TA_RXE_WAIT;
		dwt_setrxaftertxdelay(0);
		dwt_setrxtimeout(0);
	}
}



#endif


/* ==========================================================

Notes:

Previously code handled multiple instances in a single console application

Now have changed it to do a single instance only. With minimal code changes...(i.e. kept [instance] index but it is always 0.

Windows application should call instance_init() once and then in the "main loop" call instance_run().

*/
