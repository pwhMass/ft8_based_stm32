/*
 * ft8_waveform.h
 *
 *  Created on: Nov 24, 2024
 *      Author: peng
 */

#ifndef FT8_FT8_WAVEFORM_H_
#define FT8_FT8_WAVEFORM_H_

#include "main.h"

#define FT8_SYMBOL_TIME_s 0.16f
#define FT8_SYMBOL_BIN_Hz 6.25
#define FT8_FS_Hz (HSI_VALUE / Tim2_Count_Max)
#define FT8_F0_Hz 100
#define FT8_GAUSS_WINDOW_BT 2
#define GFSK_CONST_K 5.336446f
#define FT8_SYMBOL_SAMPLES ((uint32_t)(FT8_SYMBOL_TIME_s * FT8_FS_Hz + 0.5f))

void ft8_block_compute(uint32_t symIdx, uint8_t *buffer_real, uint8_t *buffer_imag);

#define FT_SYMBOL_NUMBER 79

#endif /* FT8_FT8_WAVEFORM_H_ */
