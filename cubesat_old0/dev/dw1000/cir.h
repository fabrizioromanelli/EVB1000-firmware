/*
 * Copyright (c) 2018, University of Trento.
 * All rights reserved.
 *
 */

/*
 * \file
 *    CIR Printing Functions
 *
 * \author
 *    Pablo Corbalan <p.corbalanpelegrin@unitn.it>
 */

#include "contiki.h"
#include "deca_regs.h"
/*---------------------------------------------------------------------------*/
/* Number of samples in the accumulator register depending on PRF */
/* Each sample is formed by a 16-bit real + 16-bit imaginary number */
#define ACC_LEN_PRF16 992
#define ACC_LEN_PRF64 1016
/*---------------------------------------------------------------------------*/
/* TO DO: Adapt ACC_LEN_BYTES at runtime depending on PRF configuration */
#ifdef ACC_LEN_BYTES_CONF
#define ACC_LEN_BYTES ACC_LEN_BYTES_CONF
#else
#define ACC_LEN_BYTES (ACC_LEN_PRF64 * 4)
#endif
/*---------------------------------------------------------------------------*/
#define ACC_READ_STEP (128)
/*---------------------------------------------------------------------------*/
void print_cir(void);
void print_cir_samples(uint16_t s1, uint16_t len);
void print_readable_cir(void);
/*---------------------------------------------------------------------------*/
