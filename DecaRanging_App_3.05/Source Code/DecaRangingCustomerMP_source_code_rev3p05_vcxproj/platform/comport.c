/*! ----------------------------------------------------------------------------
 * @file	comport.c
 * @brief	Decawave Com port functions
 *
 * @attention
 *
 * Copyright 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */

#include "comport.h"
#include "stdafx.h"

static HANDLE hPort = INVALID_HANDLE_VALUE;

// Open COM port
DWORD
PxSerialOpen( const char* device  )
{
    // Assume no error
    DWORD error = 0;
	DCB dcb;
	COMMTIMEOUTS cto;
    do
        {
        // Get handle to COM port
        hPort = CreateFileA(
            device,
            GENERIC_READ | GENERIC_WRITE,
            0,                                  // Exclusive access
            0,                                  // Handle not inheritable
            OPEN_EXISTING, //only open if the port exists
            0,                                  // No overlapped IO
            0                                   // No template file
            );

        if ( hPort == INVALID_HANDLE_VALUE )
            {
            error = GetLastError();
            break;
            }

        

		dcb.DCBlength = sizeof(DCB); // Initialize the DCBlength member, needed before calling Get/Set Comm State functions!!!

        // Read current port parameters
        if ( GetCommState( hPort, &dcb ) == 0 )
            {
            error = GetLastError();
            break;
            }

        // Change port parameters as needed
        dcb.BaudRate        = CBR_115200;
        //dcb.BaudRate        = CBR_9600;
		dcb.ByteSize        = 8;
        dcb.Parity          = NOPARITY;
        dcb.StopBits        = ONESTOPBIT;
        dcb.fParity         = FALSE;
        dcb.fBinary         = TRUE;                     // Windows requires this to be TRUE

        // Enable RTS/CTS handshaking
        dcb.fOutxCtsFlow    = FALSE;                     // CTS used for output flow control
        dcb.fRtsControl     = RTS_CONTROL_HANDSHAKE;    // Enable RTS handshaking

        // Disable DTR/DSR handshaking
        dcb.fOutxDsrFlow    = FALSE;                    // DSR not used for output flow control
        dcb.fDsrSensitivity = FALSE;                    // Ignore DSR
        dcb.fDtrControl     = DTR_CONTROL_ENABLE;       // Turn on DTR upon device open

        // Disable XON/XOFF handshaking
        dcb.fOutX           = FALSE;                    // No outbound XON/XOFF flow control
        dcb.fInX            = FALSE;                    // No inbound XON/XOFF flow control

        // Error handling
        dcb.fErrorChar      = FALSE;                    // No replacement of bytes with parity error
        dcb.fNull           = FALSE;                    // Don't discard NULL bytes
        dcb.fAbortOnError   = FALSE;

        // Set port parameters
        if ( SetCommState( hPort, &dcb ) == 0 )
            {
            error = GetLastError();
            break;
            }

        

        // Get port timeout parameters
        if ( GetCommTimeouts( hPort, &cto ) == 0 )
            {
            error = GetLastError();
            break;
            }

        // A value of MAXDWORD, combined with zero values for both the
        // ReadTotalTimeoutConstant and ReadTotalTimeoutMultiplier members,
        // specifies that the read operation is to return immediately with the
        // bytes that have already been received, even if no bytes have been
        // received.

        cto.ReadIntervalTimeout           = MAXDWORD;
        cto.ReadTotalTimeoutMultiplier    = 0;
        cto.WriteTotalTimeoutMultiplier   = 0;
        cto.WriteTotalTimeoutConstant     = 0;

        // Set port timeout parameters
        if ( SetCommTimeouts( hPort, &cto ) == 0 )
            {
            error = GetLastError();
            break;
            }
        }
    while( 0 );

    return error;
}

//
DWORD
PxSerialIsOpen( )
{
	 return (hPort != INVALID_HANDLE_VALUE); //open
}

// Close COM port
DWORD
PxSerialClose( )
    {
    DWORD error = 0;

    if ( hPort != INVALID_HANDLE_VALUE )
        {
        if ( CloseHandle( hPort ) == 0 )
            {
            error = GetLastError();
            }
        else
            hPort = INVALID_HANDLE_VALUE;
        }

    return error;
    }


// Read from COM port
//
// This function returns any data available at the port when
// the call is made; it does not wait.
DWORD
PxSerialRead( char* buffer, const DWORD length, DWORD* const read )
    {
    DWORD error = 0;

    if ( !ReadFile( hPort, buffer, length, read, NULL ) )
        {
        error = GetLastError();
        }

    return error;
    }


// Write to COM port
//
// This function waits until all data is written to the port.
DWORD
PxSerialWrite( char* buffer, const DWORD length, DWORD* const written )
    {
    DWORD error = 0;

    if ( !WriteFile( hPort, buffer, length, written, NULL ) )
        {
        error = GetLastError();
        }

    return error;
    }

