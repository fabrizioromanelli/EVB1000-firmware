/*! ----------------------------------------------------------------------------
 * @file	deca_spi.h.h
 * @brief	SPI Interface Access Functions
 *
 * @attention
 *
 * Copyright 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */

#ifndef _DECA_SPI_H_
#define _DECA_SPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SPI_CHEETAH_SUPPORT				(1)					// enable SPI over Cheetah USBtoSPI driver support
#define SPI_VCP_SUPPORT					(1)					// enable SPI over Virtual COM port support

#define DECA_MAX_SPI_HEADER_LENGTH      (3)                 // max number of bytes in header (for formating & sizing)

// -------------------------------------------------------------------------------------------------------------------
// Low level abstract function to open and initialise access to the SPI device.
// Returns 0 if successful or -1 in case of an error.

int openspi(void) ;

int openspi_vcp(void);
int openspi_ch(void);

// -------------------------------------------------------------------------------------------------------------------
// Low level abstract function to close the SPI device.
// returns 0 for success, or -1 for error

int closespi(void) ;

int closespi_vcp(void);
int closespi_ch(void);

// -------------------------------------------------------------------------------------------------------------------
//

int setspibitrate(int desiredRatekHz) ;
int setspibitrate_ch(int desiredRatekHz) ;
int setspibitrate_vcp(int desiredRatekHz) ;

int spilogenable(int enable);                       // run time enable/disable logging of SPI activity to a file

#ifdef __cplusplus
}
#endif

#endif /* _DECA_SPI_H_ */



