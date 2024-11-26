/*
 * dac_controller.c
 *
 *  Created on: Nov 24, 2024
 *      Author: peng
 */

#include "dac_controller.h"

extern DMA_HandleTypeDef hdma_dac_ch1;
extern DMA_HandleTypeDef hdma_dac_ch2;
extern DAC_HandleTypeDef hdac1;
extern TIM_HandleTypeDef htim2;
struct
{
	uint8_t *buffer; // 双缓冲 需要长度大于等于blockSize*4
	uint32_t blockSize;
	uint8_t currentBuffer; // 当前正在被DMA填充的缓冲区
	uint8_t bufferReady;   // 数据就绪标志
	uint32_t blockIdx;	   // 当前位于第几个block
	uint32_t blockNum;	   // 所需要传输数据的block的个数
	void (*computeBlock)(uint32_t blockIdx, uint8_t *buffer1_ptr, uint8_t *buffer2_ptr);
} DMA_Buffer;

void reset_buffer()
{
	DMA_Buffer.currentBuffer = 0;
	DMA_Buffer.bufferReady = 0;
	DMA_Buffer.blockIdx = 0;
}

#define ONE_BUFFER_SIZE (DMA_Buffer.blockSize * 2)

// 定义回调函数
void DmaTransferCallback(DAC_HandleTypeDef *hdac)
{
	DMA_HandleTypeDef *hdma1 = hdac->DMA_Handle1;
	if (hdma1->Instance == DMA1_Channel3)
	{
		// USE FOR DEBUG
		if (DMA_Buffer.bufferReady == 0)
		{
			while (1)
			{
				// 可以在这里添加LED闪烁等提示
				HAL_GPIO_TogglePin(GPIOB, LD3_Pin | LD2_Pin);
				HAL_Delay(1000);
			}
		}

		// 设置state
		DMA_Buffer.currentBuffer ^= 1; // 切换到另一个缓冲区
		DMA_Buffer.blockIdx += 1;
		DMA_Buffer.bufferReady = 0; // 设置数据就绪标志

		// 结束发送数据
		if (DMA_Buffer.blockIdx == DMA_Buffer.blockNum)
		{
			HAL_TIM_Base_Stop(&htim2);
			HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
			HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_2);
			DMA_Buffer.bufferReady = 1;
		}
	}
}

void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
	DmaTransferCallback(hdac);
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
	DmaTransferCallback(hdac);
}

void DacControllerInit(uint8_t *buffer, uint32_t blockSize, uint32_t blockNum,
					   void (*computeBlock)(uint32_t blockIdx, uint8_t *buffer1_ptr, uint8_t *buffer2_ptr))
{
	HAL_TIM_Base_Stop(&htim2);
	HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
	HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_2);
	DMA_Buffer.buffer = buffer;
	DMA_Buffer.blockSize = blockSize;
	DMA_Buffer.blockNum = blockNum;
	DMA_Buffer.computeBlock = computeBlock;
	reset_buffer();
}

void DAC_Controller_Process()
{
	if (!DMA_Buffer.bufferReady)
	{
		uint8_t processBuffer = DMA_Buffer.currentBuffer ^ 1;

		// 获取processBuffer
		uint8_t *buffer1_ptr, *buffer2_ptr;
		if (processBuffer == 0)
		{
			buffer1_ptr = DMA_Buffer.buffer;
			buffer2_ptr = DMA_Buffer.buffer + DMA_Buffer.blockSize * 2;
		}
		else
		{
			buffer1_ptr = DMA_Buffer.buffer + DMA_Buffer.blockSize;
			buffer2_ptr = DMA_Buffer.buffer + DMA_Buffer.blockSize * 3;
		}

		// 数据计算，当超出一块时清零
		if (DMA_Buffer.blockIdx + 1 < DMA_Buffer.blockNum)
		{
			DMA_Buffer.computeBlock(DMA_Buffer.blockIdx + 1, buffer1_ptr, buffer2_ptr);
		}
		else
		{
			// 数据清零
			for (uint32_t i = 0; i < DMA_Buffer.blockSize; i++)
			{
				buffer1_ptr[i] = 0;
				buffer2_ptr[i] = 0;
			}
		}

		// update state
		DMA_Buffer.bufferReady = 1;
	}
}

void DAC_Controller_Start()
{
	reset_buffer();
	DMA_Buffer.computeBlock(0, DMA_Buffer.buffer, DMA_Buffer.buffer + DMA_Buffer.blockSize * 2);

	// 需要注意HAL_DAC_Start_DMA接受uint32_t *pData，需要正确设置DMA每次传送位数

	HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t *)DMA_Buffer.buffer, ONE_BUFFER_SIZE, DAC_ALIGN_8B_R);
	HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_2, (uint32_t *)(DMA_Buffer.buffer + DMA_Buffer.blockSize * 2), ONE_BUFFER_SIZE, DAC_ALIGN_8B_R);
	HAL_TIM_Base_Start(&htim2);
}