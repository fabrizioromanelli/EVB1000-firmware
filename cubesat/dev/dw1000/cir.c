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


#include "cir.h"
#include "contiki.h"
#include "deca_regs.h"
#include "deca_device_api.h"
/*---------------------------------------------------------------------------*/
#include <stdio.h>
/*---------------------------------------------------------------------------*/
static uint8_t acc[ACC_READ_STEP + 1] = {0};
/*---------------------------------------------------------------------------*/
void
print_cir(void)
{
  uint16_t data_len = 0;

  printf("Acc Data: ");
  for(uint16_t j = 0; j < ACC_LEN_BYTES; j = j + ACC_READ_STEP) {
    /* Select the number of bytes to read from the accummulator */
    data_len = ACC_READ_STEP;
    if (j + ACC_READ_STEP > ACC_LEN_BYTES) {
      data_len = ACC_LEN_BYTES - j;
    }
    dwt_readaccdata(acc, data_len + 1, j);

    for(uint16_t k = 1; k < data_len + 1; k++) {
      printf("%02x", acc[k]);
    }
  }
  printf("\n");
}
/*---------------------------------------------------------------------------*/
void
print_cir_samples(uint16_t s1, uint16_t len)
{
  uint16_t data_len = 0;
  uint16_t nbytes = (len < ACC_READ_STEP) ? len : ACC_READ_STEP;
  uint16_t max_bytes = (s1 + len < ACC_LEN_BYTES) ? (s1 + len) : ACC_LEN_BYTES;

  printf("Acc Data: ");
  for(uint16_t j = s1; j < max_bytes; j = j + nbytes) {
    /* Select the number of bytes to read from the accummulator */
    data_len = nbytes;
    if (j + nbytes > max_bytes) {
      data_len = max_bytes - j;
    }
    dwt_readaccdata(acc, data_len + 1, j);

    for(uint16_t k = 1; k < data_len + 1; k++) {
      printf("%02x", acc[k]);
    }
  }
  printf("\n");
}
/*---------------------------------------------------------------------------*/
void
print_readable_cir(void)
{
  uint16_t data_len = 0;
  int16_t a = 0; /* Real part */
  int16_t b = 0; /* Imaginary part */

  printf("Acc Data: ");
  for(int j = 0; j < ACC_LEN_BYTES; j = j + ACC_READ_STEP) {
    /* Select the number of bytes to read from the accummulator */
    data_len = ACC_READ_STEP;
    if (j + ACC_READ_STEP > ACC_LEN_BYTES) {
      data_len = ACC_LEN_BYTES - j;
    }
    dwt_readaccdata(acc, data_len + 1, j);

    /* Print the bytes read as complex numbers */
    for(int k = 1; k < data_len + 1; k = k + 4) {
      a = (int16_t) (((acc[k + 1] & 0x00FF) << 8) | (acc[k] & 0x00FF));
      b = (int16_t) (((acc[k + 3] & 0x00FF) << 8) | (acc[k + 2] & 0x00FF));
      if(b >= 0) {
        printf("%d+%dj,", a, b);
      } else {
        printf("%d%dj,", a, b);
      }
    }
  }
  printf("\n");
}
/*---------------------------------------------------------------------------*/
