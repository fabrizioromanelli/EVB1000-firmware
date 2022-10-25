/*! ----------------------------------------------------------------------------
 * @file	comport.h
 * @brief	Decawave Com port function definitions
 *
 * @attention
 *
 * Copyright 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */

#ifndef __COMPORT_H__
#define __COMPORT_H__
#ifdef __cplusplus
extern "C" {
#endif


#include "stdafx.h"

// Open COM port
DWORD
PxSerialOpen(
	const char* device  // COM port. e.g. "COM1"
    );

// Is the COM port already open
DWORD
PxSerialIsOpen( );

// Close COM port
DWORD
PxSerialClose( );

// Read from COM port
//
// This function returns any data available at the port when
// the call is made; it does not wait.
DWORD
PxSerialRead(
	char* buffer,       // Buffer to read data into
    const DWORD length, // Size of buffer in bytes
    DWORD* const read         // Bytes read from COM port
    );

// Write to COM port
//
// This function waits until all data is written to the port.
DWORD
PxSerialWrite(
	char* buffer,       // Buffer containing data to write
    const DWORD length, // Size of data in bytes
    DWORD* const written      // Bytes written to COM port
    );

#ifdef __cplusplus
}
#endif

#endif
