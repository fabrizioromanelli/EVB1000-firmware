/*! ----------------------------------------------------------------------------
 * @file	deca_vcspi.h
 * @brief	Virtual COM port (USB to SPI)
 *
 * @attention
 *
 * Copyright 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */

#ifndef _DECA_VCSPI_H_
#define _DECA_VCSPI_H_

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    int com_port;
    int hispeed;           //if 1 it is high speed, 0 it is slow
    int connected;
	int dw1000;
	int usbrev;

} usbtospiConfig_t ;

int openCOMport(int port);

int closeCOMport(int port);

int writeCOMdata(char* buffer, int length, int *);

int readCOMdata(char* buffer,  int buffer_size, int *);

int findandopenEVB1000COMport(void);

int vcspi_setspibitrate(int desiredRatekHz);
#ifdef __cplusplus
}
#endif

#endif //_DECA_VCSPI_H_

