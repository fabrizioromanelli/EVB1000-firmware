/*! ----------------------------------------------------------------------------
 * @file	deca_spinit.h
 * @brief	SPI Interface Initialisation definitions
 *
 * @attention
 *
 * Copyright 2013 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 */

#ifndef _DECA_SPINIT_H_
#define _DECA_SPINIT_H_

#ifdef __cplusplus
extern "C" {
#endif

// rate in kHz (1000 == 1Mbit/s)
#define DW_DEFAULT_SPI		(3000) //3MHz
#define DW_FAST_SPI			(20000) //20MHz

int spilogenable(int enable);                       // run time enable/disable logging of SPI activity to a file

#ifdef __cplusplus
}
#endif

#endif



