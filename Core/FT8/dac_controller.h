/*
 * dac_controller.h
 *
 *  Created on: Nov 24, 2024
 *      Author: peng
 */

#ifndef FT8_DAC_CONTROLLER_H_
#define FT8_DAC_CONTROLLER_H_

#include "main.h"

// 初始化
void DacControllerInit(uint8_t *buffer, uint32_t blockSize, uint32_t blockNum,
                       void (*computeBlock)(uint32_t blockIdx, uint8_t *buffer1_ptr, uint8_t *buffer2_ptr));

// 开启传送，会自动关闭
void DAC_Controller_Start();

// 在main函数中要循环调用，用于计算并加载数据
void DAC_Controller_Process();

#endif /* FT8_DAC_CONTROLLER_H_ */
