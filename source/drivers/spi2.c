#include "board.h"
#include "spi2.h"

static volatile uint16_t spiErrorCount2 = 0;

void spiInit2(void)
{
	SPI_InitTypeDef spi;
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitTypeDef GPIO_InitStructure1;
	GPIO_InitTypeDef GPIO_InitStructure2;
	GPIO_InitTypeDef GPIO_InitStructure3;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

	GPIO_PinAFConfig(GPIOE, GPIO_PinSource10, GPIO_AF_SPI2);

	GPIO_StructInit(&GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	//GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOE, &GPIO_InitStruct);

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	//GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOD, &GPIO_InitStruct);

	GPIOE->BSRRL = GPIO_Pin_10; // set PE10 high

	SPI_I2S_DeInit(SPI_BUSE2);

	// enable clock for used IO pins
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	GPIO_PinAFConfig(SPI_GPIO_2, SPI_SCK_PIN_SOURCE_2, GPIO_AF_SPI2);
	GPIO_PinAFConfig(SPI_GPIO_2, SPI_MISO_PIN_SOURCE_2, GPIO_AF_SPI2);
	GPIO_PinAFConfig(SPI_GPIO_2, SPI_MOSI_PIN_SOURCE_2, GPIO_AF_SPI2);

	// Init pins
	GPIO_InitStructure1.GPIO_Pin = SPI_SCK_PIN_2;
	GPIO_InitStructure1.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure1.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure1.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure1.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(SPI_GPIO_2, &GPIO_InitStructure1);

	GPIO_InitStructure2.GPIO_Pin = SPI_MISO_PIN_2;
	GPIO_InitStructure2.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure2.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure2.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure2.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(SPI_GPIO_2, &GPIO_InitStructure2);

	GPIO_InitStructure3.GPIO_Pin = SPI_MOSI_PIN_2;
	GPIO_InitStructure3.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure3.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure3.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure3.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(SPI_GPIO_2, &GPIO_InitStructure3);

	SPI_I2S_DeInit(SPI_BUSE2);

	// Enable SPI1 clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

	SPI_InitTypeDef SPI_InitStructure;
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI_BUSE2, &SPI_InitStructure);

	SPI_CalculateCRC(SPI_BUSE2, DISABLE);

	SPI_Cmd(SPI_BUSE2, ENABLE);
}

// return uint8_t value or -1 when failure
uint8_t spiTransferByte2(uint8_t data)
{
    uint16_t spiTimeout = 1000;

    while (SPI_I2S_GetFlagStatus(SPI_BUSE2, SPI_I2S_FLAG_TXE) == RESET)
        if ((spiTimeout--) == 0)
            return spiTimeoutUserCallback2();

    SPI_SendData(SPI_BUSE2, data);

    spiTimeout = 1000;
    while (SPI_I2S_GetFlagStatus(SPI_BUSE2, SPI_I2S_FLAG_RXNE) == RESET)
        if ((spiTimeout--) == 0)
            return spiTimeoutUserCallback2();

    return ((uint8_t)SPI_ReceiveData(SPI_BUSE2));
}

// return true or -1 when failure
uint8_t spiTransfer2(uint8_t *out, uint8_t *in, int len)
{
    uint16_t spiTimeout;
    uint8_t b;

    while (len--) {
        b = in ? *(in++) : 0xFF;
        spiTimeout = 1000;
        while (SPI_I2S_GetFlagStatus(SPI_BUSE2, SPI_I2S_FLAG_TXE) == RESET) {
            if ((spiTimeout--) == 0)
                return spiTimeoutUserCallback2();
        }
        SPI_SendData(SPI_BUSE2, b);
        spiTimeout = 1000;
        while (SPI_I2S_GetFlagStatus(SPI_BUSE2, SPI_I2S_FLAG_RXNE) == RESET) {
            if ((spiTimeout--) == 0)
                return spiTimeoutUserCallback2();
        }

        b = SPI_ReceiveData(SPI_BUSE2);
        if (out)
            *(out++) = b;
    }
    return 1;
}



///////////////////////////////////////////////////////////////////////////////
// Set SPI Divisor
///////////////////////////////////////////////////////////////////////////////

void setSPIdivisor2(uint16_t divisor)
{
#define BR_CLEAR_MASK 0xFFC7

    uint16_t tempRegister;

    SPI_Cmd(SPI_BUSE2, DISABLE);

    tempRegister = SPI_BUSE2->CR1;

    switch (divisor) {
        case 2:
            tempRegister &= BR_CLEAR_MASK;
            tempRegister |= SPI_BaudRatePrescaler_2;
            break;

        case 4:
            tempRegister &= BR_CLEAR_MASK;
            tempRegister |= SPI_BaudRatePrescaler_4;
            break;

        case 8:
            tempRegister &= BR_CLEAR_MASK;
            tempRegister |= SPI_BaudRatePrescaler_8;
            break;

        case 16:
            tempRegister &= BR_CLEAR_MASK;
            tempRegister |= SPI_BaudRatePrescaler_16;
            break;

        case 32:
            tempRegister &= BR_CLEAR_MASK;
            tempRegister |= SPI_BaudRatePrescaler_32;
            break;

        case 64:
            tempRegister &= BR_CLEAR_MASK;
            tempRegister |= SPI_BaudRatePrescaler_64;
            break;

        case 128:
            tempRegister &= BR_CLEAR_MASK;
            tempRegister |= SPI_BaudRatePrescaler_128;
            break;

        case 256:
            tempRegister &= BR_CLEAR_MASK;
            tempRegister |= SPI_BaudRatePrescaler_256;
            break;
    }

    SPI_BUSE2->CR1 = tempRegister;

    SPI_Cmd(SPI_BUSE2, ENABLE);
}


uint32_t spiTimeoutUserCallback2(void)
{
	spiErrorCount2++;
    return -1;
}

uint16_t spiGetErrorCounter2(void)
{
	return spiErrorCount2;
}

void spiResetErrorCounter2(void)
{
	spiErrorCount2 = 0;
}
