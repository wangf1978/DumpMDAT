#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__

#include "Bitstream.h"
#include <stdint.h>

int8_t huffman_scale_factor(CBitstream& bs);
uint8_t huffman_spectral_data(uint8_t cb, CBitstream& bs, int16_t *sp);
#ifdef ERROR_RESILIENCE
int8_t huffman_spectral_data_2(uint8_t cb, bits_t *ld, int16_t *sp);
#endif

#endif
