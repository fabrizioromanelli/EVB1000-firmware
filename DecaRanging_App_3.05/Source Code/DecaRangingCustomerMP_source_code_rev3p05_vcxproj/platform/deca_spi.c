/*! ------------------------------------------------------------------------------------------------------------------
 * @file	 deca_spi.c
 * @brief	 SPI Interface Access Functions
 *
 * @attention
 * 
 * Copyright 2008, 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */

// -------------------------------------------------------------------------------------------------------------------
//
//  When targeting to a precessor with SPI this file should be replaced with appropriate target SPI control file.
//
//  Here for PC implementation we use Cheetah USB to SPI convertor from Total Phase Inc.
//
// -------------------------------------------------------------------------------------------------------------------

#include "cheetah.h"                                            // we use Cheetah USB to SPI board to talk to SPI.
#include "deca_device_api.h"
#include "deca_spi.h"
#include "deca_spinit.h"
#include "deca_vcspi.h"

#define DECA_SPI_DEBUG_LOG_ENABLE       (0)     // COMPILE TIME DEBUG LOG ENABLE (SO CREATES FILE AND LOGS FROM VERY FIRST TRANSACTION)
#define DECA_SPI_APP_LOG_SUPPORT        (1)     // allow application to turn logging on/off (set to 0 to remove support)

#if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
#include <stdio.h>
#include <string.h>
#include "deca_util.h"
#define DECA_SPI_DEBUG_LOG_FILE      "_DecaWaveSPI.log"         // file name (tail) to use, date string is prefixed
static int logOpenRequired = 1 ;
static FILE *spiLog = NULL ;
#include "DecaRanging_Ver.h"
static char * build = __DATE__  ", " __TIME__ ;
#endif

int spihandle = -1;		//this is the handle used for Cheetah SPI functions
int vcphandle = -1;		//this is the handle used for VCP SPI functions
extern usbtospiConfig_t usbtospiConfig;

// -------------------------------------------------------------------------------------------------------------------
#if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
// This internal functions is used when compile-time or run-time debug log enable is needed
int spilogopen(void)
{
    char fnbuf[sizeof(DECA_SPI_DEBUG_LOG_FILE)+DECA_STAMP_STRING_LENGTH] ;       // declare buffer for file name
    char *s = filldatetimestring(fnbuf,sizeof(fnbuf)) ;                         // begin filename with date code and ...
    strcpy_s(s,sizeof(fnbuf)-(s-fnbuf),DECA_SPI_DEBUG_LOG_FILE) ;               // then tag on rest of file name
    if (fopen_s(&spiLog,fnbuf,"wt") != 0)                                       // open the file
    {
        printf("\n\aERROR - LOG FILE OPEN FAILED.\a\n\n") ;
        spiLog = NULL ;         // to be sure
        return DWT_ERROR ;
    }

    fprintf(spiLog,"File:%s, created by %s (build:%s)\n",fnbuf,SOFTWARE_VER_STRING,build);

    return DWT_SUCCESS ;
}
#endif

// This functions is called from the application when run-time debug log enable is needed
int spilogenable(int enable)
{
#if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT == 0)   
    // if compile tome debug log is on then no stoppping/starting is allowed.
    // or if runtime debug log enable is not supported then don't allow it
    return -1 ;                                                 // file creation error
#else
    
    if (spiLog != NULL)                                 // close any active log
    {
         fclose(spiLog) ;
         spiLog = NULL ;
    }

    if (enable == 0) return 0 ;                         // disabling logging, nothing else to do
    
    return spilogopen() ;                               // Open new log file

#endif
}


// -------------------------------------------------------------------------------------------------------------------
// Low level abstract function to open and initialise access to the SPI device.
//
// Sets the local handle for use in talking to device and returns succes (0) or -1 in case of an error.
//
int openspi(void)
{
	int handle = DWT_ERROR;

#if DECA_SPI_DEBUG_LOG_ENABLE==1
    if (logOpenRequired)
    {
        logOpenRequired = 0 ;
        if(spilogopen())                // if open fails
        {
            return DWT_ERROR ;
        }
    }
#endif

#if (SPI_VCP_SUPPORT == 1)
	handle = openspi_vcp();	//try over VCP

	if(handle != DWT_ERROR)
	//if handle & 0x100 - we are using VCP 
	if((handle & 0xF00) >= 0x100)
	{
		vcphandle = handle;
	}
#endif

#if (SPI_CHEETAH_SUPPORT == 1)
	//if no VCP
	if(handle == DWT_ERROR)
	{
		handle = openspi_ch();

		if(handle != DWT_ERROR)
			spihandle = handle;
	}
#endif
	return handle;
} // end openspi()

int openspi_vcp(void)
{
	int opened ; 

	opened = findandopenEVB1000COMport(); //find a COM port connected to EVB1000

	#if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
    if (spiLog != NULL)
    {
		if(usbtospiConfig.connected)
			fprintf(spiLog,"Connected to EVB1000 device on COM %d\n", usbtospiConfig.com_port); 
        else
			fprintf(spiLog,"No EVB1000 devices found on COM ports\n"); 
        fflush(spiLog);
    }
    #endif

	return opened ;
} // end openspi_vcp()

int openspi_ch(void)
{

    u16 ports[16];
    u32 unique_ids[16];
    int nelem = 16;
    int i;
    int handle ;
    int count ;
    int r ;
    int bitrate ;

    // Code below searches for cheetah devices and uses the first one that is free,
    // another strategy would be to grab one by a supplied port number parameter.
    // In a target system with fixed SPI device this function becomes much simpler.
    // - simply grab/init the SPI device. See "Configure for operating mode" and "Set the bitrate" below.


    // Find all the attached Cheetah devices

    count = ch_find_devices_ext(nelem, ports, nelem, unique_ids);
    if (count > nelem)  count = nelem;


    #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
    if (spiLog != NULL)
    {
        fprintf(spiLog,"%d device(s) found:\n", count); fflush(spiLog);
    }
    #endif
    if (count <= 0) return DWT_ERROR ;

    // find free device....
    for (i = 0; i < count; ++i)
    {

        if (!(ports[i] & CH_PORT_NOT_FREE))
        {
            CheetahExt ch_ext ;

            // found a free one

            #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
            if (spiLog != NULL)
            {
                fprintf(spiLog,"free device %i  port=%-3d (%04d-%06d)\n",i, ports[i], unique_ids[i]/1000000,unique_ids[i]%1000000);
            }
            #endif

            // open device
            handle = ch_open_ext (ports[i], &ch_ext);
            // printf("opened port %i handle returned %i\n",i,handle) ;

            if (handle <= 0)
            {
                #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
                if (spiLog != NULL)
                {
                    fprintf(spiLog,"Unable to open Cheetah device on port - Error code = %d\n", handle); fflush(spiLog);
                }
                #endif

                return DWT_ERROR;
            }

            #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
            if (spiLog != NULL)
            {
                fprintf(spiLog,"Opened Device (Handle %i)\n"
                               "              Software version: %04X\n"
                               "              Hardware version: %04X\n"
                               "              Hardware rev req: %08X\n"
                               "              Driver revsn req: %08X\n"
                               "              API required    : %04X\n"
                               "              Features: %d\n\n",
                               handle,
                               ch_ext.version.software,
                               ch_ext.version.hardware,
                               ch_ext.version.hw_revs_for_sw,
                               ch_ext.version.drv_revs_for_sw,
                               ch_ext.version.api_req_by_sw,
                               ch_ext.features) ;

                fprintf(spiLog,"Host interface is %s\n",(ch_host_ifce_speed(handle)) ? "high speed" : "full speed");
            }
            #endif



            // Configure for operating mode

            // ---------------------------------------------------------------------------------------------------
            // NB: For DecaWave SPI we need the follwoing Cheetah setup.  If replacing this function with a target
            //     specific SPI please refer to DecaWave Device Manual for details of SPI requirements and then as
            //     configure your SPI hardware accordingly.
            // ---------------------------------------------------------------------------------------------------
#if 1 //(mode 0)
            r = ch_spi_configure(handle,
                             CH_SPI_POL_RISING_FALLING,         // Clock idle low, active high
                             CH_SPI_PHASE_SAMPLE_SETUP,         // Sample on the leading edge of the clock signal
                             CH_SPI_BITORDER_MSB,               // Most significant bit first
                             0x00) ;                            // QActive low Selects.
#elif 0 //(mode 2)
            r = ch_spi_configure(handle,
                             CH_SPI_POL_FALLING_RISING,         // Clock idle high, active low
                             CH_SPI_PHASE_SAMPLE_SETUP,         // Sample on the leading edge of the clock signal
                             CH_SPI_BITORDER_MSB,               // Most significant bit first
                             0x00) ;                            // QActive low Selects.
#elif 0 //(mode 1)
            r = ch_spi_configure(handle,
                             CH_SPI_POL_RISING_FALLING,         // Clock idle low, active high
                             CH_SPI_PHASE_SETUP_SAMPLE,         // Sample on the trailing edge of the clock signal
                             CH_SPI_BITORDER_MSB,               // Most significant bit first
                             0x00) ;                            // QActive low Selects.
#else //(mode 3)
            r = ch_spi_configure(handle,
                             CH_SPI_POL_FALLING_RISING,         // Clock idle high, active low
                             CH_SPI_PHASE_SETUP_SAMPLE,         // Sample on the trailing edge of the clock signal
                             CH_SPI_BITORDER_MSB,               // Most significant bit first
                             0x00) ;                            // QActive low Selects.
#endif			
			
			
			
			
			if (r != CH_OK)
            {
                #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
                if (spiLog != NULL)
                {
                    fprintf(spiLog,"ch_spi_configure Error code = %d\n", r); fflush(spiLog);
                }
                #endif

                return DWT_ERROR;
            }

            #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
            if (spiLog != NULL)
            {
                fprintf(spiLog,"SPI configured okay.\n");
            }
            #endif

            // Power the device using the Cheetah adapter's power supply.
            r = ch_target_power(handle, CH_TARGET_POWER_OFF);   // NO POWER SUPPLIED FOR PROTOTYPE
            if (r != CH_TARGET_POWER_OFF)
            {
                #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
                if (spiLog != NULL)
                {
                    fprintf(spiLog,"ch_target_power off failed\n"); fflush(spiLog);
                }
                #endif

                return DWT_ERROR;
            }

            ch_sleep_ms(100);                                   // sleep (to allow power to settle ?) for ON option

            // Set the bitrate.

            // NB: Recommend using highest bitrate supported by SPI and DecaWave Device, (Ref: Device Manual)

            bitrate = ch_spi_bitrate(handle, DW_DEFAULT_SPI);   // number is kbits/s, start with 1Mbits/s

            if (bitrate < 0)
            {
                #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
                if (spiLog != NULL)
                {
                    fprintf(spiLog,"Could not set bitrate. (Error code = %d).\n", bitrate); fflush(spiLog);
                }
                #endif

                ch_close (handle)  ;                            // Close the Cheetah port
                return DWT_ERROR;
            }

            #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
            if (spiLog != NULL)
            {
                fprintf(spiLog,"SPI Bitrate set to %d kHz\n", bitrate);
                fflush(spiLog);
            }
            #endif

            // return handle for using and closing ?
            return handle ;

        }
    }

    return DWT_ERROR ;                                         // no free device found

} // end openspi_ch()


// -------------------------------------------------------------------------------------------------------------------
// Low level abstract function to select SPI bit rate.
//
//
// called directly by windows application
//

int setspibitrate(int desiredRatekHz)
{
	int result = DWT_ERROR;
#if (SPI_CHEETAH_SUPPORT == 1)
	if(spihandle != DWT_ERROR)
		result = setspibitrate_ch(desiredRatekHz);
	else
#endif
#if (SPI_VCP_SUPPORT == 1)
	if(vcphandle != DWT_ERROR)
		result = setspibitrate_vcp(desiredRatekHz);
#endif	
	return result;
}

int setspibitrate_vcp(int desiredRatekHz)
{
	int x = vcspi_setspibitrate(desiredRatekHz);

	if (usbtospiConfig.hispeed)
    {
        #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
        if (spiLog != NULL)
        {
            fprintf(spiLog,"Set high speed rate\n"); fflush(spiLog);
        }
        #endif                                       // some error
    }
	else
	{
		#if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
		if (spiLog != NULL)
		{
			fprintf(spiLog,"Set low speed rate\n");
			fflush(spiLog);
		}
		#endif
	}
	return x;
}

int setspibitrate_ch(int desiredRatekHz)
{
    int bitrate = 0;
	int handle = spihandle;

    bitrate = ch_spi_bitrate(handle, desiredRatekHz);             // number is kbits/s, start with 1Mbits/s

    if (bitrate < 0)
    {
        #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
        if (spiLog != NULL)
        {
            fprintf(spiLog,"Could not set bitrate in setspibitrate(). Error code = %d.\n", bitrate); fflush(spiLog);
        }
        #endif

        return DWT_ERROR ;                                         // some error
    }

    #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
    if (spiLog != NULL)
    {
        fprintf(spiLog,"SPI Bitrate (desired %d) set to %d kHz\n", desiredRatekHz, bitrate);
        fflush(spiLog);
    }
	#endif

	return bitrate ;

} // end setspibitrate()

// -------------------------------------------------------------------------------------------------------------------
// Low level abstract function to close the SPI device.
//
// returns 0 for success, or -1 for error
//
int closespi(void)
{
	int result = DWT_ERROR;

#if (SPI_CHEETAH_SUPPORT == 1)
	if(spihandle != DWT_ERROR)
	{
		result = closespi_ch();
		spihandle = -1;
	}
#endif

#if (SPI_VCP_SUPPORT == 1)
	if(vcphandle != DWT_ERROR)
	{
		result = closespi_vcp();	//try over VCP
		vcphandle = -1;
	}
#endif

	return result;
} // end closespi()

int closespi_vcp(void)
{
    #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
    if (spiLog != NULL)
    {
        fprintf(spiLog,"\n\n>>>>> SPI Close %s - (Handle %i)\n", (1 == 1) ? "Success." : " ERROR !!!", vcphandle );
    }
    #endif

    #if DECA_SPI_DEBUG_LOG_ENABLE==1
    logOpenRequired = 1;
    fclose(spiLog) ;
    #endif

	return closeCOMport((vcphandle & 0xFF)); //find a COM port connected to EVB1000
} // end closespi_vcp()

int closespi_ch(void)
{
	int handle = spihandle;
    int numberClosed = ch_close(handle);                        // Close cheetah device

    #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
    if (spiLog != NULL)
    {
        fprintf(spiLog,"\n\n>>>>> SPI Close %s - (Handle %i)\n", (numberClosed == 1) ? "Success." : " ERROR !!!", handle );
    }
    #endif

    #if DECA_SPI_DEBUG_LOG_ENABLE==1
    logOpenRequired = 1;
    fclose(spiLog) ;
    #endif

    if (numberClosed == 1) return DWT_SUCCESS ;

    return DWT_ERROR ;

} // end closespi_ch()



// -------------------------------------------------------------------------------------------------------------------
// Low level abstract function to write to the SPI
//
// Takes two separate byte buffers for write header and write data
//
// returns 0 for success, or -1 for error
//

int writetospi_vcp
(
    uint16       headerLength,
    const uint8 *headerBuffer,
    uint32       bodylength,
    const uint8 *bodyBuffer
)
{
	int error;
	int size = 0;
	int written = 0;
	int read = 0;
	//first byte specifies the SPI speed and SPI read/write operation
	//bit 0 = 1 for write, 0 for read
	//bit 1 = 1 for high speed, 0 for low speed

	//<STX>   <ETX>
	//to write to the device (e.g. 4 bytes),  total length = 1 + 1 + len_of_command (2) + len_of_data_to_write (2) + header len (1/2) + data len + 1
	//LBS comes first:   0x2, 0x2, 0x7, 0x0, 0xb6, 0x1, 0x3 

	uint8 *txBuffer ;
	uint8 *rxBuffer ;
	int totallength = 7 + bodylength + headerLength;
    
	#if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
    unsigned int i ;
    if (spiLog != NULL)
    {
        fprintf(spiLog,"%i-Write [",vcphandle);
        fflush(spiLog);
    }
    #endif

	txBuffer = (uint8*) malloc(totallength);
	rxBuffer = (uint8*) malloc(3);

	txBuffer[0] = 0x2;
	txBuffer[1] = 0x1 | (usbtospiConfig.hispeed << 1);
	txBuffer[2] = (totallength & 0xFF);
	txBuffer[3] = (totallength >> 8) & 0xFF;
	txBuffer[4] = (bodylength & 0xFF);
	txBuffer[5] = (bodylength >> 8) & 0xFF;

	memcpy(&txBuffer[6], headerBuffer, headerLength);

    #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
    if (spiLog != NULL)
    {
        for (i = 0 ; i < headerLength ; i++)
            fprintf(spiLog,"%02X",headerBuffer[i]);
        for (      ; i < DECA_MAX_SPI_HEADER_LENGTH ; i++)
            fprintf(spiLog,"  ",headerBuffer[i]);
        fprintf(spiLog,"] ");
    }
    #endif

	memcpy(&txBuffer[6+headerLength], bodyBuffer, bodylength);

    #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
    if (spiLog != NULL)
    {
        for (i = 0 ; i < bodylength ; i++)
            fprintf(spiLog,"%02X ",bodyBuffer[i]);
    }
    #endif

	txBuffer[totallength - 1] = 0x3;

	//send the data over VCP to ARM processor
	error = writeCOMdata((char*) txBuffer, totallength, &written);

	if(error != 0)
	{
		free(rxBuffer);
		free(txBuffer);
		return DWT_ERROR;
	}

	error = readCOMdata((char*) rxBuffer , 3, &read);

	if(error != 0)
	{
		free(rxBuffer);
		free(txBuffer);
		return DWT_ERROR;
	}

	if(read != 3)
	{
		printf("WE/RD SPI error\n");
		error = DWT_ERROR;
	}
	else if((rxBuffer[0] == 0x2) && (rxBuffer[1] == 0x0) && (rxBuffer[2] == 0x3))
	{
		//all OK the read was sucessfull
		#if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
		if (spiLog != NULL)
		{
			fprintf(spiLog,"<OK>\n");
			fflush(spiLog);
		}
		#endif
		error = DWT_SUCCESS;
	}
	else
	{
		uint16 index = (headerLength == 1) ? 0 : ((txBuffer[6+1] & 0x80) ? ((txBuffer[6+2] << 7) | (txBuffer[6+1] & 0x7F) ) : (txBuffer[6+1] & 0x7F));
		printf("WE SPI error: reg 0x%02X %04X\n", txBuffer[6] & 0x3F, index);
		error = DWT_ERROR;
	}

	free(txBuffer);
	free(rxBuffer);

	return error;

} // end writetospi_vcp()

int writetospi_ch
(
    uint16       headerLength,
    const uint8 *headerBuffer,
    uint32       bodylength,
    const uint8 *bodyBuffer
)
{
	int handle = spihandle ;
    #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
    unsigned int i ;
    if (spiLog != NULL)
    {
        fprintf(spiLog,"%i-Write [",handle);
        fflush(spiLog);
    }
    #endif

    // Prepare for transaction.  Transactions are queued so we have to queue all items.

    if (ch_spi_queue_clear(handle) != CH_OK) return DWT_ERROR;     // clear out any previous transaction.
    if (ch_spi_queue_oe(handle,1) != CH_OK) return DWT_ERROR;      // ensure outputs are enabled
    if (ch_spi_queue_ss(handle,7) != CH_OK) return DWT_ERROR;      // set select line, bit 0 is select line 0, value 1 sets it active (low)

    // Write message header

    if (ch_spi_queue_array(handle,headerLength,headerBuffer)!= headerLength) return DWT_ERROR; // Queue the array to be sent

    #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
    if (spiLog != NULL)
    {
        for (i = 0 ; i < headerLength ; i++)
            fprintf(spiLog,"%02X",headerBuffer[i]);
        for (      ; i < DECA_MAX_SPI_HEADER_LENGTH ; i++)
            fprintf(spiLog,"  ",headerBuffer[i]);
        fprintf(spiLog,"] ");
    }
    #endif

    // Write message body

    if (ch_spi_queue_array(handle,bodylength,bodyBuffer) != bodylength) return DWT_ERROR; // Queue the array to be sent

    #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
    if (spiLog != NULL)
    {
        for (i = 0 ; i < bodylength ; i++)
            fprintf(spiLog,"%02X ",bodyBuffer[i]);
    }
    #endif

    // end by de-asserting the select line

    if (ch_spi_queue_ss(handle,0) != CH_OK) return DWT_ERROR;      // set select line-0 inactive again

    // now queue is filled with what we want to send.  So we can send it.

    if ( ch_spi_batch_shift (handle,0,(void *)0)                    // shift queued bytes/line changes out SPI, capture no bytes in return
         != headerLength + bodylength) return DWT_ERROR ;          // and check it went okay

    #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
    if (spiLog != NULL)
    {
        fprintf(spiLog,"<OK>\n");
        fflush(spiLog);
    }
    #endif

    // printf(" SPI Write \n") ; 

    return DWT_SUCCESS ;                                           // hurrah

} // end writetospi_ch()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: writetospi()
 *
 * Description:  
 * NB: In porting this to a particular microporcessor, the implementer needs to define the two low
 * level abstract functions to write to and read from the SPI the definitions should be in deca_spi.c file. 
 * Low level abstract function to write to the SPI
 * Takes two separate byte buffers for write header and write data
 * returns 0 for success, or -1 for error
 *
 * Note: The body of this function is defined in deca_spi.c and is platform specific
 *
 * input parameters:	
 * @param headerLength	- number of bytes header being written 
 * @param headerBuffer	- pointer to buffer containing the 'headerLength' bytes of header to be written
 * @param bodylength	- number of bytes data being written 
 * @param bodyBuffer	- pointer to buffer containing the 'bodylength' bytes od data to be written
 *
 * output parameters
 *
 * returns DWT_DECA_SUCCESS for success, or DWT_DECA_ERROR for error
 */
int writetospi
(
    uint16       headerLength,
    const uint8 *headerBuffer,
    uint32       bodylength,
    const uint8 *bodyBuffer
)
{
	int result = DWT_ERROR;

#if (SPI_CHEETAH_SUPPORT == 1)
	if(spihandle != DWT_ERROR)
	{
		result = writetospi_ch(headerLength, headerBuffer, bodylength, bodyBuffer);
	}
	else
#endif

#if (SPI_VCP_SUPPORT == 1)
	if(vcphandle != DWT_ERROR)
	{
		result = writetospi_vcp(headerLength, headerBuffer, bodylength, bodyBuffer);	//try over VCP
	}
#endif

	if(result == DWT_ERROR)
		exit(1);

	return result;
} // end writetospi()

// -------------------------------------------------------------------------------------------------------------------
//  Low level abstract function to read from the SPI
//
//  Takes two separate byte buffers for write header and read data
//
//  returns the offset into read buffer where first byte of read data may be found.
//  (for the Cheetah this offset is actually the headerLength)
//
//

int readfromspi_vcp
(
    uint16       headerLength,
    const uint8 *headerBuffer,
    uint32       readlength,
    uint8       *readBuffer
)
{
	int error;
	int size = 0;
	int written = 0;
	int read = 0;
	//first byte specifies the SPI speed and SPI read/write operation
	//bit 0 = 1 for write, 0 for read
	//bit 1 = 1 for high speed, 0 for low speed

	//<STX>   <ETX>
	//e.g. to read from the device (e.g. 4 bytes), total length is = 1 + 1 + length_of_command (2) + length_of_data_to_read (2) + header length (1/2) + 1
	//LBS comes first:   0x2, 0x2, 0x7, 0x0, 0xb6, 0x1, 0x3 

	uint8 *txBuffer ;
	uint8 *rxBuffer ;
	int totallength = 2/*readlength value*/ + headerLength + 5;

    #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
    unsigned int i ;
    if (spiLog != NULL)
    {
        fprintf(spiLog,"%i-Read: [",vcphandle);
        fflush(spiLog);
    }
    #endif

	txBuffer = (uint8*) malloc(totallength);
	rxBuffer = (uint8*) malloc(readlength + 3);

	txBuffer[0] = 0x2;
	txBuffer[1] = (usbtospiConfig.hispeed << 1);
	txBuffer[2] = (totallength & 0xFF);
	txBuffer[3] = (totallength >> 8) & 0xFF;
	txBuffer[4] = (readlength & 0xFF);
	txBuffer[5] = (readlength >> 8) & 0xFF;

	memcpy(&txBuffer[6], headerBuffer, headerLength);
    #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
    if (spiLog != NULL)
    {
        for (i = 0 ; i < headerLength ; i++)
            fprintf(spiLog,"%02X",headerBuffer[i]);
        for (      ; i < DECA_MAX_SPI_HEADER_LENGTH ; i++)
            fprintf(spiLog,"  ",headerBuffer[i]);
        fprintf(spiLog,"] ");
    }
    #endif

	txBuffer[totallength - 1] = 0x3;

	//send the data over VCP to ARM processor
	error = writeCOMdata((char*) txBuffer, totallength, &written);

	if(error != 0)
	{
		free(rxBuffer);
		free(txBuffer);
		return DWT_ERROR;
	}

	error = readCOMdata((char*) rxBuffer, readlength+3, &read);
	
	if(error != 0)
	{
		free(rxBuffer);
		free(txBuffer);
		return DWT_ERROR;
	}

	if(read != (readlength+3))
	{
		int left = (readlength+3) - read;
		int readPtr = read;
			
		error = writeCOMdata((char*)"deca?", 5, &written);
		
		Sleep(2);

		while(left > 0)
		{
			error = readCOMdata((char*) rxBuffer+readPtr, left, &read);
			readPtr += read;
			left -= read;
		}

		{
			char tmp[50];
			int r;
			readCOMdata(tmp, 22, &r);
		}

		error = DWT_SUCCESS;
	}

	if((rxBuffer[0] == 0x2) && (rxBuffer[1] == 0x0) && (rxBuffer[readlength+2] == 0x3))
	{
		#if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
		if (spiLog != NULL)
		{
			for (i = 2 ; i < readlength+2 ; i++)
			{
				fprintf(spiLog,"%02X ",rxBuffer[i]);     // log each received byte
			}
			fprintf(spiLog,"<OK>\n");
			fflush(spiLog);
		}
		#endif
		//all OK the read was sucessfull
		memcpy(readBuffer, &rxBuffer[2], readlength);
		error = DWT_SUCCESS;
	}
	else
	{
		int x = 0;
		uint16 index = (headerLength == 1) ? 0 : ((txBuffer[6+1] & 0x80) ? ((txBuffer[6+2] << 7) | (txBuffer[6+1] & 0x7F) ) : (txBuffer[6+1] & 0x7F));
		printf("RD SPI error: reg 0x%02X %04X (read %d / %d)\n", txBuffer[6] & 0x3F, index, read, readlength+3);
		printf("rxBuffer[last3] = %02X %02X %02X\n", rxBuffer[readlength], rxBuffer[readlength+1], rxBuffer[readlength+2]);
		//error = readCOMdata((char*) rxBuffer, readlength+3, &read);

		for(x = 0; x < (int)(readlength+3); x+=4)
		{
			printf("%02X %02X %02X %02X\n", rxBuffer[x], rxBuffer[x+1], rxBuffer[x+2], rxBuffer[x+3]);
		}

		if(read == (readlength+3)) 
			error = DWT_SUCCESS;
		else
			error = DWT_ERROR;
	}
	free(rxBuffer);
	free(txBuffer);

	return error;

}// end readfromspi_vcp()


int readfromspi_ch
(
    uint16       headerLength,
    const uint8 *headerBuffer,
    uint32       readlength,
    uint8       *readBuffer
)
{
	int handle = spihandle ;
    int res ;
	uint8 *rxBuffer ;

    #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
    unsigned int i ;
    if (spiLog != NULL)
    {
        fprintf(spiLog,"%i-Read: [",handle);
        fflush(spiLog);
    }
    #endif

	rxBuffer = (uint8*) malloc(readlength+headerLength);

    // Prepare for transaction.  Transactions are queued so we have to queue all items.

    if (CH_OK != (res = ch_spi_queue_clear(handle))) return DWT_ERROR;     // clear out any previous transaction.
    if (CH_OK != (res = ch_spi_queue_oe(handle,1))) return DWT_ERROR;      // ensure outputs are enabled
    if (CH_OK != (res = ch_spi_queue_ss(handle,7))) return DWT_ERROR;      // set select line, bit 0 is select line 0, value 1 sets it active (low)


    // Write message header

    if (ch_spi_queue_array(handle,headerLength,headerBuffer)!= headerLength) return DWT_ERROR; // Queue the array to be sent

    #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
    if (spiLog != NULL)
    {
        for (i = 0 ; i < headerLength ; i++)
            fprintf(spiLog,"%02X",headerBuffer[i]);
        for (      ; i < DECA_MAX_SPI_HEADER_LENGTH ; i++)
            fprintf(spiLog,"  ",headerBuffer[i]);
        fprintf(spiLog,"] ");
    }
    #endif

    // Write dummy message body - we need to shift something out so that we can shift data in....

    if (ch_spi_queue_byte(handle,readlength,0x00)!=readlength) return DWT_ERROR;   // send dummy bytes (zeros) for length we want to read back

    // end by de-asserting the select line

    if (ch_spi_queue_ss(handle,0) != CH_OK) return DWT_ERROR;          // set select line-0 inactive again

    // now queue is filled with what we want to send (and receive at same time), let's send it.

    if ( ch_spi_batch_shift (handle,readlength+headerLength,rxBuffer) // shift queued bytes/line changes out SPI, capture RX butes into buffer
         != readlength+headerLength) return DWT_ERROR ;                // and check it went okay

    #if (DECA_SPI_DEBUG_LOG_ENABLE==1)||(DECA_SPI_APP_LOG_SUPPORT==1)
    if (spiLog != NULL)
    {
        for (i = 0 ; i < readlength+headerLength ; i++)
        {
            fprintf(spiLog,"%02X%s",rxBuffer[i],(i == headerLength-1) ? " - " : " ");     // log each received byte
        }
        fprintf(spiLog,"<OK>\n");
        fflush(spiLog);
    }
    #endif

	for (i = 0 ; i < (readlength) ; i++)
    {
		readBuffer[i] = rxBuffer[headerLength + i];
	}

	free(rxBuffer);
    // printf(" SPI Read \n") ; 

    //return headerLength ;                                               // all okay return offset
	return DWT_SUCCESS;

} // end readfromspi_ch()

/*! ------------------------------------------------------------------------------------------------------------------
 * Function: readfromspi()
 *
 * Description:  
 * NB: In porting this to a particular microporcessor, the implementer needs to define the two low
 * level abstract functions to write to and read from the SPI the definitions should be in deca_spi.c file. 
 * Low level abstract function to write to the SPI
 * Takes two separate byte buffers for write header and write data
 * returns 0 for success, or -1 for error
 *
 * Note: The body of this function is defined in deca_spi.c and is platform specific
 *
 * input parameters:	
 * @param headerLength	- number of bytes header to write 
 * @param headerBuffer	- pointer to buffer containing the 'headerLength' bytes of header to write
 * @param bodylength	- number of bytes data being read 
 * @param bodyBuffer	- pointer to buffer containing to return the data (NB: size required = headerLength + readlength)
 *
 * output parameters
 *
 * returns DWT_DECA_SUCCESS for success (and the position in the buffer at which data begins), or DWT_DECA_ERROR for error
 */
int readfromspi
(
    uint16       headerLength,
    const uint8 *headerBuffer,
    uint32       readlength,
    uint8       *readBuffer
)
{
	int result = DWT_ERROR;

#if (SPI_CHEETAH_SUPPORT == 1)
	if(spihandle != DWT_ERROR)
	{
		result = readfromspi_ch(headerLength, headerBuffer, readlength, readBuffer);
	}
	else
#endif

#if (SPI_VCP_SUPPORT == 1)
	if(vcphandle != DWT_ERROR)
	{
		result = readfromspi_vcp(headerLength, headerBuffer, readlength, readBuffer);	//try over VCP
	}
#endif


	if(result == DWT_ERROR)
		exit(1);

	return result;
}// end readfromspi()
