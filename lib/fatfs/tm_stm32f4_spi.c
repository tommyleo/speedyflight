/**	
 * |----------------------------------------------------------------------
 * | Copyright (C) Tilen Majerle, 2014
 * | 
 * | This program is free software: you can redistribute it and/or modify
 * | it under the terms of the GNU General Public License as published by
 * | the Free Software Foundation, either version 3 of the License, or
 * | any later version.
 * |  
 * | This program is distributed in the hope that it will be useful,
 * | but WITHOUT ANY WARRANTY; without even the implied warranty of
 * | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * | GNU General Public License for more details.
 * | 
 * | You should have received a copy of the GNU General Public License
 * | along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * |----------------------------------------------------------------------
 */
#include "tm_stm32f4_spi.h"

extern void TM_SPI3_Init(TM_SPI_Mode_t SPI_Mode);

void TM_SPI_Init() {
	TM_SPI3_Init(TM_SPI3_MODE);
}

void TM_SPI_InitWithMode(TM_SPI_Mode_t SPI_Mode) {
	TM_SPI3_Init(SPI_Mode);
}

uint8_t TM_SPI_Send(SPI_TypeDef* SPIx, uint8_t data) {
	/* Fill output buffer with data */
	SPIx->DR = data;
	/* Wait for transmission to complete */
	while (!SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_TXE));
	/* Wait for received data to complete */
	while (!SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_RXNE));
	/* Wait for SPI to be ready */
	while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_BSY));
	/* Return data from buffer */
	return SPIx->DR;
}

void TM_SPI_SendMulti(SPI_TypeDef* SPIx, uint8_t* dataOut, uint8_t* dataIn, uint16_t count) {
	uint16_t i;
	for (i = 0; i < count; i++) {
		dataIn[i] = TM_SPI_Send(SPIx, dataOut[i]);
	}
}

void TM_SPI_WriteMulti(SPI_TypeDef* SPIx, uint8_t* dataOut, uint16_t count) {
	uint16_t i;
	for (i = 0; i < count; i++) {
		TM_SPI_Send(SPIx, dataOut[i]);
	}
}

void TM_SPI_ReadMulti(SPI_TypeDef* SPIx, uint8_t* dataIn, uint8_t dummy, uint16_t count) {
	uint16_t i;
	for (i = 0; i < count; i++) {
		dataIn[i] = TM_SPI_Send(SPIx, dummy);
	}
}

uint16_t TM_SPI_Send16(SPI_TypeDef* SPIx, uint16_t data) {
	/* Fill output buffer with data */
	SPIx->DR = data;
	/* Wait for transmission to complete */
	while (!SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_TXE));
	/* Wait for received data to complete */
	while (!SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_RXNE));
	/* Wait for SPI to be ready */
	while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_BSY));
	/* Return data from buffer */
	return SPIx->DR;
}

void TM_SPI_SendMulti16(SPI_TypeDef* SPIx, uint16_t* dataOut, uint16_t* dataIn, uint16_t count) {
	uint16_t i;
	for (i = 0; i < count; i++) {
		dataIn[i] = TM_SPI_Send16(SPIx, dataOut[i]);
	}
}

void TM_SPI_WriteMulti16(SPI_TypeDef* SPIx, uint16_t* dataOut, uint16_t count) {
	uint16_t i;
	for (i = 0; i < count; i++) {
		TM_SPI_Send16(SPIx, dataOut[i]);
	}
}

void TM_SPI_ReadMulti16(SPI_TypeDef* SPIx, uint16_t* dataIn, uint16_t dummy, uint16_t count) {
	uint16_t i;
	for (i = 0; i < count; i++) {
		dataIn[i] = TM_SPI_Send16(SPIx, dummy);
	}
}

void TM_SPI3_Init(TM_SPI_Mode_t SPI_Mode) {
	GPIO_InitTypeDef GPIO_InitStruct;
	SPI_InitTypeDef SPI_InitStruct;

	//Common settings for all pins
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;

	//Enable clock
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	//Pinspack nr. 1        SCK          MISO         MOSI
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_Init(GPIOC, &GPIO_InitStruct);

	GPIO_PinAFConfig(GPIOC, GPIO_PinSource10, GPIO_AF_SPI3);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource11, GPIO_AF_SPI3);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource12, GPIO_AF_SPI3);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3, ENABLE);

	SPI_StructInit(&SPI_InitStruct);
	SPI_InitStruct.SPI_BaudRatePrescaler = TM_SPI3_PRESCALER;
	SPI_InitStruct.SPI_DataSize = TM_SPI3_DATASIZE;
	SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStruct.SPI_FirstBit = TM_SPI3_FIRSTBIT;
	SPI_InitStruct.SPI_Mode = TM_SPI3_MASTERSLAVE;
	if (SPI_Mode == TM_SPI_Mode_0) {
		SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
		SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
	} else if (SPI_Mode == TM_SPI_Mode_1) {
		SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
		SPI_InitStruct.SPI_CPHA = SPI_CPHA_2Edge;
	} else if (SPI_Mode == TM_SPI_Mode_2) {
		SPI_InitStruct.SPI_CPOL = SPI_CPOL_High;
		SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
	} else if (SPI_Mode == TM_SPI_Mode_3) {
		SPI_InitStruct.SPI_CPOL = SPI_CPOL_High;
		SPI_InitStruct.SPI_CPHA = SPI_CPHA_2Edge;
	}
	SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
	
	SPI_Cmd(SPI3, DISABLE);
	SPI_DeInit(SPI3);
	
	SPI_Init(SPI3, &SPI_InitStruct);
	SPI_Cmd(SPI3, ENABLE);
}
