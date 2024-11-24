/*
 * dac_controller.c
 *
 *  Created on: Nov 24, 2024
 *      Author: peng
 */

#include "dac_controller.h"

extern DMA_HandleTypeDef hdma_dac_ch1;
extern DAC_HandleTypeDef hdac1;
volatile struct
{
	uint8_t *buffer; // 双缓冲 需要长度大于等于blockSize*2
	uint32_t blockSize;
	uint8_t currentBuffer; // 当前正在被DMA填充的缓冲区
	uint8_t bufferReady;   // 数据就绪标志
	uint32_t blockIdx;	   // 当前位于第几个block
	uint32_t blockNum;	   // 所需要传输数据的block的个数
	void (*computeBlock)(uint32_t blockIdx, uint8_t *buffer_ptr);
} DMA_Buffer;

void reset_buffer()
{
	DMA_Buffer.currentBuffer = 0;
	DMA_Buffer.bufferReady = 0;
	DMA_Buffer.blockIdx = 0;
}

inline uint32_t buffer_size()
{
	return DMA_Buffer.blockSize * 2;
}

// 定义回调函数
void DmaTransferCallback(DMA_HandleTypeDef *hdma)
{

	if (hdma->Instance == DMA1_Channel1)
	{

		// USE FOR DEBUG
		if (DMA_Buffer.bufferReady == 0)
		{
			/* 数据未处理完，触发panic */
			Error_Handler(); // STM32 HAL库的错误处理函数
			while (1)
			{
				// 可以在这里添加LED闪烁等提示
				HAL_GPIO_TogglePin(GPIOB, LD3_Pin | LD2_Pin);
				HAL_Delay(100);
			}
		}

		// 设置state
		DMA_Buffer.currentBuffer ^= 1; // 切换到另一个缓冲区
		DMA_Buffer.blockIdx += 1;
		DMA_Buffer.bufferReady = 0; // 设置数据就绪标志

		// 结束发送数据
		if (DMA_Buffer.blockIdx == DMA_Buffer.blockNum)
		{

			HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
			DMA_Buffer.bufferReady = 1;
		}
	}
}

void DacControllerInit(uint8_t *buffer, uint32_t bufferSize, uint32_t blockNum, void (*computeBlock)(uint32_t blockIdx, uint8_t *buffer_ptr))
{
	DMA_Buffer.buffer = buffer;
	DMA_Buffer.blockSize = bufferSize;
	DMA_Buffer.blockNum = blockNum;
	DMA_Buffer.computeBlock = computeBlock;
	reset_buffer();

	HAL_DMA_RegisterCallback(&hdma_dac_ch1, HAL_DMA_XFER_HALFCPLT_CB_ID, DmaTransferCallback);
	HAL_DMA_RegisterCallback(&hdma_dac_ch1, HAL_DMA_XFER_CPLT_CB_ID, DmaTransferCallback);
}

void DAC_Controller_Process()
{
	if (!DMA_Buffer.bufferReady)
	{
		uint8_t processBuffer = DMA_Buffer.currentBuffer ^ 1;

		// 获取processBuffer
		uint8_t *buffer_ptr;
		if (processBuffer == 0)
		{
			buffer_ptr = DMA_Buffer.buffer;
		}
		else
		{
			buffer_ptr = DMA_Buffer.buffer + DMA_Buffer.blockSize;
		}

		// 数据计算，当超出一块时清零
		if (DMA_Buffer.blockIdx + 1 < DMA_Buffer.blockNum)
		{
			DMA_Buffer.computeBlock(DMA_Buffer.blockIdx + 1, buffer_ptr);
		}
		else
		{
			// 数据清零
			for (uint32_t i = 0; i < DMA_Buffer.blockSize; i++)
			{
				buffer_ptr[i] = 0;
			}
		}

		// update state
		DMA_Buffer.bufferReady = 1;
	}
}

void DAC_Controller_Start()
{
	reset_buffer();
	DMA_Buffer.computeBlock(0, DMA_Buffer.buffer);

	// 需要注意HAL_DAC_Start_DMA接受uint32_t *pData，需要正确设置DMA每次传送位数
	HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, DMA_Buffer.buffer, buffer_size(), DAC_ALIGN_8B_R);
}
