/*
 * ft8_waveform.c
 *
 *  Created on: Nov 24, 2024
 *      Author: peng
 */

#include "ft8_waveform.h"
#include "math.h"
#include <stdint.h>

#define PI 3.14159265358979323846f

uint8_t float_to_uint8(float x)
{
	uint16_t result = (uint16_t)(((x) + 1.0f) * 128.0f);
	if (result > 255)
		return 255;
	else
		return result;
}

static const uint8_t itones[79] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	4, 4, 4, 4};

void gfsk_pulse(float *pulse)
{
	for (uint32_t i = 0; i < 3 * FT8_SYMBOL_SAMPLES; ++i)
	{
		float t = i / (float)FT8_SYMBOL_SAMPLES - 1.5f;
		float arg1 = GFSK_CONST_K * FT8_GAUSS_WINDOW_BT * (t + 0.5f);
		float arg2 = GFSK_CONST_K * FT8_GAUSS_WINDOW_BT * (t - 0.5f);
		pulse[i] = (erff(arg1) - erff(arg2)) / 2;
	}
}

static uint32_t gauss_window_is_init = 0;
static float gauss_window[3 * FT8_SYMBOL_SAMPLES];

void synth_gfsk(const uint8_t *symbols, uint32_t n_sym, float f0, uint32_t out_sym, uint8_t *output_signal_real, uint8_t *output_signal_imag)
{

	// 初始化高斯窗口
	if (!gauss_window_is_init)
	{
		gfsk_pulse(gauss_window);
		gauss_window_is_init = 1;
	}

	float hmod = 1.0f;
	float dphi_peak = 2.0f * PI * hmod / FT8_SYMBOL_SAMPLES;

	// 获取需要的三个符号
	uint8_t needed_sym[3];
	if (out_sym == 0)
	{
		needed_sym[0] = symbols[0];
		needed_sym[1] = symbols[0];
		needed_sym[2] = symbols[1];
	}
	else if (out_sym == n_sym - 1)
	{
		needed_sym[0] = symbols[n_sym - 2];
		needed_sym[1] = symbols[n_sym - 1];
		needed_sym[2] = symbols[n_sym - 1];
	}
	else
	{
		needed_sym[0] = symbols[out_sym - 1];
		needed_sym[1] = symbols[out_sym];
		needed_sym[2] = symbols[out_sym + 1];
	}

	// 生成波形
	static float phi = 0.0f;
	float base_dphi = 2.0f * PI * f0 / FT8_FS_Hz;

	for (uint32_t k = 0; k < FT8_SYMBOL_SAMPLES; ++k)
	{

		// 生成IQ信号
		output_signal_real[k] = float_to_uint8(cosf(phi));
		output_signal_imag[k] = float_to_uint8(sinf(phi));
		// 计算相位增量
		float ext_dphi = dphi_peak * (needed_sym[0] * gauss_window[k + 2 * FT8_SYMBOL_SAMPLES] +
									  needed_sym[1] * gauss_window[k + FT8_SYMBOL_SAMPLES] +
									  needed_sym[2] * gauss_window[k]);

		// 更新相位并限制在[0, 2π]范围内
		phi += base_dphi + ext_dphi;
		while (phi >= 2.0f * PI)
		{
			phi -= 2.0f * PI;
		}
		while (phi < 0.0f)
		{
			phi += 2.0f * PI;
		}
	}

	// 应用包络整形
	if ((out_sym == 0) || (out_sym == (n_sym - 1)))
	{
		uint32_t n_ramp = FT8_SYMBOL_SAMPLES / 8;
		for (uint32_t i = 0; i < n_ramp; ++i)
		{
			float env = (1.0f - cosf(PI * i / n_ramp)) / 2.0f;
			if (out_sym == 0)
			{
				output_signal_real[i] = float_to_uint8(env * (output_signal_real[i] - 128.0f) / 128.0f);
				output_signal_imag[i] = float_to_uint8(env * (output_signal_imag[i] - 128.0f) / 128.0f);
			}
			else
			{
				output_signal_real[FT8_SYMBOL_SAMPLES - 1 - i] = float_to_uint8(env * (output_signal_real[FT8_SYMBOL_SAMPLES - 1 - i] - 128.0f) / 128.0f);
				output_signal_imag[FT8_SYMBOL_SAMPLES - 1 - i] = float_to_uint8(env * (output_signal_imag[FT8_SYMBOL_SAMPLES - 1 - i] - 128.0f) / 128.0f);
			}
		}
	}
}

void ft8_block_compute(uint32_t symIdx, uint8_t *buffer_real, uint8_t *buffer_imag)
{
	synth_gfsk(itones, FT_SYMBOL_NUMBER, FT8_F0_Hz, symIdx, buffer_real, buffer_imag);
}
