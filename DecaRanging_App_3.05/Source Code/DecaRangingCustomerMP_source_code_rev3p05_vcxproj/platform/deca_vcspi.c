/*! ------------------------------------------------------------------------------------------------------------------
 * @file	deca_vcspi.c
 * @brief	Virtual COM port (USB to SPI)
 *
 * @attention
 * 
 * Copyright 2008, 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */

#include "deca_vcspi.h"
#include "deca_device_api.h"
#include "comport.h"
#include "stdio.h"

#define DEBUGPRINT (0)

usbtospiConfig_t usbtospiConfig = {     24, //default COM port
                                         0, //hispeed (1 == high speed)
                                         0, //connected
										 0, //DW1000 device 
										 0
                                  };

//open specified COM port
int openCOMport(int port)
{
    int error = 0;
    char buffer[ 256 ];
    DWORD written = 0;

    if(port > 9)
    {
        sprintf_s( buffer, 20, "\\\\.\\COM%d", port );
        // !!!! if COM port > 9 need to use \\.\ before it or "\\\\.\\" with escape chars (Windows feature) !!!!
    }
    else
    {
        sprintf_s( buffer, 20, "COM%d", port );
    }

    // Open port
    error = PxSerialOpen((const char*) buffer );

    if ( error != 0 )
    {
        printf( "Error %08x opening %d.\n", error, port );
        return 1;
    }

	printf( "Port %d %08x open.\n", port, error);
    usbtospiConfig.com_port = port;

    return 0;
}

#define EVB1000STRA ("EVB1000 USB2SPI 1.0")
#define EVB1000STRB ("EVB1000 USB2SPI 2.0")
#define OCANCHORSTR ("REK1101 USB2SPI 1.1")	//628 and 707 type 1 (fine grain seq off)
#define OCTAGSTR	("REK1101 USB2SPI 1.0")

int isEVB1000device(void)
{
	int i = 0;
	int error = 0;
	char buffer[ 1024 ];
	int size = 0;
	int waitfor = sizeof(EVB1000STRB) + 2; //"deca?" response string length is 22

	memset(buffer, 0, 1024);

	//send 5 bytes to the COM port and wait for response
	error = writeCOMdata("deca?", 5, &size);

	//read back the string sent by the EVB1000 USB to SPI application
	Sleep(2);
	do
	{
		error = readCOMdata(buffer + i, 1, &size);

		//if ( (buffer[1] == 'O') && (buffer[20] == '0') ) break;
		if (buffer[i] == 'y' )  //reset buffer
		{ 
			buffer[0] = 'y'; 
			i = 0; 
			waitfor = 22;
		}
		if (buffer[i] == 'i')  //reset buffer - we are connecting to a tag/anchor that is already outputting range information
		{
			buffer[0] = buffer[i]; 
			i = 0; 
			waitfor = 42;
		}
		if (buffer[i] == 'n' )  //reset buffer
		{ 
			buffer[0] = 'n'; 
			i = 0; 
			waitfor = 22;
		}		
		
		i = (i + 1) % 1024;

		if(i == waitfor) 
			break;

		if(error) break;

	} while (1);
	
	//check if the read response is "yEVB1000 USB to SPI 1.0"
	if(buffer[0] == 'y')
	{
		if(strcmp(&buffer[1],EVB1000STRA) == 0)
		{
			printf("Connected to EVB1000, rev:<%s>\n", &buffer[1]);
			usbtospiConfig.connected = 1;
			usbtospiConfig.dw1000 = 1;
			usbtospiConfig.usbrev = 1;
			return 1;
		}
		else
		if(strcmp(&buffer[1],EVB1000STRB) == 0)
		{
			printf("Connected to EVB1000, rev:<%s>\n", &buffer[1]);
			usbtospiConfig.connected = 1;
			usbtospiConfig.dw1000 = 1;
			usbtospiConfig.usbrev = 2;
			return 1;
		}
		else
		if(strcmp(&buffer[1],OCANCHORSTR) == 0)
		{
			printf("Connected to Anchor, rev:<%s>\n", &buffer[1]);
			usbtospiConfig.connected = 1;
			usbtospiConfig.dw1000 = 2;
			usbtospiConfig.usbrev = 2;
			return 1;
		}
		else
		if (strcmp(&buffer[1], OCTAGSTR) == 0)
		{
			printf("Connected to Tag, rev:<%s>\n", &buffer[1]);
			usbtospiConfig.connected = 1;
			usbtospiConfig.dw1000 = 3;
			usbtospiConfig.usbrev = 2;
			return 1;
		}
	}
#if 0
	else if(buffer[0] == 'n')
	{
		if(strcmp(&buffer[1],EVB1000STRB) == 0)
		{
			uint8 buf[6];
			uint16 rxant = 0;
			uint16 txant = 0;
			int read = 0;
			int sw1 = 0x0
					| 0x0;

			printf("Connected to EVB1000 on port %d, rev:<%s>\n", usbtospiConfig.com_port, &buffer[1]);
			usbtospiConfig.connected = 1;
			usbtospiConfig.dw1000 = 2;
			usbtospiConfig.usbrev = 2;
			//return 1;
			
			
#if 0
			//antenna cal... 
			buf[0] = buf[5] = 0x5;
			buf[1] = txant & 0xff; 
			buf[2] = (txant >> 8) & 0xff; 
			buf[3] = rxant & 0xff; 
			buf[4] = (rxant >> 8) & 0xff; 
			//send 6 bytes to the COM port and wait for response
			error = writeCOMdata((char*) buf, 6, &size);
#else
			//S1 config
			buf[0] = buf[2] = 0x6;
			buf[1] = sw1; 
			//send 3 bytes to the COM port and wait for response
			error = writeCOMdata((char*) buf, 3, &size);
#endif
			
			
			i = 0;

			size = 0;

			Sleep(2);
			do
			{
				error = readCOMdata(buffer + i, 1, &size);
		
				if (buffer[i] == 'i') //reset buffer on 1st char ... ranging output starts with "i" and size is 40 chars.
				{ 
					buffer[0] = 'i'; 
					i = 0; 
				}
				i = (i + 1) % 1024;
				read += size;
				if(i == 42)
				{
					int w = 0;
					i = 0;
					printf("read %d chars, <%s>\n", read, buffer);
					//break;

					if(w == 1)
					{
						error = writeCOMdata((char*) buf, 6, &size);
					}

					read = 0;
				}

				if(error) break;

			} while (1);
		}
	}
#endif

	//if not as expected fail by returning 0
	{
		printf("read %d chars, <%s>\n", size, buffer);
		usbtospiConfig.connected = 0;
		usbtospiConfig.dw1000 = 0;
	}

	return 0;

}



int findandopenEVB1000COMport(void)
{
	int i;

	for(i=3; i<50; i++)
	{
		if(openCOMport(i) == 0) //i port exist and is open
		{
			if(isEVB1000device()) //check if EVB1000 is on this port
			{
				return (i | (usbtospiConfig.usbrev << 8));
			}
			else //close the COM port
			{
				closeCOMport(i);
			}

		}
	}

	return DWT_ERROR;
}



//close specified COM port
int closeCOMport(int port)
{
    int error = PxSerialClose();
    if ( error != 0 )
    {
        printf( "Error %08x closing %d.\n", error, port );
        return 1;
    }

    usbtospiConfig.connected  = 0; //we have disconnected from the controller
	usbtospiConfig.dw1000 = 0;

    return DWT_SUCCESS;
}


int writeCOMdata(char* buffer, int length, int * written)
{
    int error = 0;
#if (DEBUGPRINT == 1)
    int i;
    int len = strlen( buffer );
    printf("GPIBwr:>");
    for(i=0; i<len; i++)
    {
        if(buffer[i] == '\r')
            printf("\\r");
        else
            printf("%c",buffer[i]);
    }
    printf("<\n");
#endif
    error = PxSerialWrite( buffer, length, (DWORD*) written );         // Write command to port
	
	//Sleep(2);

    if ( error != 0 )
    {
        printf( "Error %08x writing %d.\n", error, usbtospiConfig.com_port );
        return error;
    }

    return DWT_SUCCESS;
}

int readCOMdata(char* buffer, int buffer_size, int * read)
{
	int error = 0;
	int index = 0;
	int reading = 0;
	int bytesleft = buffer_size;
	int count = 1000;
	int dwcount = 500000;
	int start = 0;

	do	//read (buffer_size) bytes from the COM port
	{
		
		reading = 0;
		error = PxSerialRead( &buffer[index], bytesleft, (DWORD*) &reading);

		if(reading > 0)
		{
			bytesleft -= reading ; //subtract the read bytes 
			index += reading ; //next index in the buffer
			
			//printf("read %d %d %d %d\n", reading, count, bytesleft, GetLastError());
			
			start = 1;
			count = 1000;
			//Sleep(2);
		}
		else if(usbtospiConfig.dw1000 != 2) //don't do timeout for OC ANCHORS
		{
			//Sleep(2);

			if(start && (count-- == 0)) { printf("ERROR: read %d %d left %d\n", reading, count, bytesleft); error = 1; break; }

			if(usbtospiConfig.dw1000 == 0) //Serial port is not DW1000/ARM 
			{
				if((start == 0) && (count-- == 0)) { printf("ERROR: port read timeout %d %d left %d\n", reading, count, bytesleft); error = 2; break; }
			}
			else //if(usbtospiConfig.dw1000 == 1)
			{
				if(((reading + start) == 0) && (dwcount-- == 0)) 
				{ 
					printf("ERROR: COM read error\n"); error = 1; break; 
				}
			}
		}
		


	}while(bytesleft > 0);

	if ( error != 0 )
    {
        printf( "Error %08x reading COM %d.\n", error, usbtospiConfig.com_port );
        return error;
    }
	*read = index;
    return DWT_SUCCESS; 
}


int vcspi_setspibitrate(int desiredRatekHz)
{
	if(desiredRatekHz < 4000) //select slow speed
	{
		usbtospiConfig.hispeed  = 0;
	}
	else //fast SPI
	{
		usbtospiConfig.hispeed  = 1;
	}

	return DWT_SUCCESS;
}