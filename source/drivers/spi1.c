#include "board.h"
#include "spi1.h"


static volatile uint16_t spiErrorCount = 0;

bool spiInit(void)
{

    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;

    RCC_AHB1PeriphClockCmd(SPI_SCK_CLK | SPI_MISO_CLK | SPI_MOSI_CLK, ENABLE);

    GPIO_PinAFConfig(SPI_GPIO, SPI_SCK_PIN_SOURCE, GPIO_AF_SPI2);
    GPIO_PinAFConfig(SPI_GPIO, SPI_MISO_PIN_SOURCE, GPIO_AF_SPI2);
    GPIO_PinAFConfig(SPI_GPIO, SPI_MOSI_PIN_SOURCE, GPIO_AF_SPI2);

    // Init pins
    GPIO_InitStructure.GPIO_Pin = SPI_SCK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

    GPIO_Init(SPI_GPIO, &GPIO_InitStructure);

    ///////////////////////////////

    GPIO_InitStructure.GPIO_Pin = EEPROM_CS_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

    GPIO_Init(EEPROM_CS_GPIO, &GPIO_InitStructure);

    DISABLE_EEPROM;

	GPIO_InitStructure.GPIO_Pin = MS5611_CS_PIN;
	GPIO_Init(MS5611_CS_GPIO, &GPIO_InitStructure);
	DISABLE_MS5611;

    SPI_I2S_DeInit(SPI_BUSE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI_BUSE, &SPI_InitStructure);

    SPI_CalculateCRC(SPI_BUSE, DISABLE);

    SPI_Cmd(SPI_BUSE, ENABLE);

    return true;
}


///////////////////////////////////////////////////////////////////////////////
// SPI Transfer
///////////////////////////////////////////////////////////////////////////////

// return uint8_t value or -1 when failure
uint8_t spiTransferByte(uint8_t data)
{
    uint16_t spiTimeout = 1000;

    while (SPI_I2S_GetFlagStatus(SPI_BUSE, SPI_I2S_FLAG_TXE) == RESET)
        if ((spiTimeout--) == 0)
            return spiTimeoutUserCallback();

    SPI_SendData(SPI_BUSE, data);

    spiTimeout = 1000;
    while (SPI_I2S_GetFlagStatus(SPI_BUSE, SPI_I2S_FLAG_RXNE) == RESET)
        if ((spiTimeout--) == 0)
            return spiTimeoutUserCallback();

    return ((uint8_t)SPI_ReceiveData(SPI_BUSE));
}

// return true or -1 when failure
uint8_t spiTransfer(uint8_t *out, uint8_t *in, int len)
{
    uint16_t spiTimeout;
    uint8_t b;

    while (len--) {
        b = in ? *(in++) : 0xFF;
        spiTimeout = 1000;
        while (SPI_I2S_GetFlagStatus(SPI_BUSE, SPI_I2S_FLAG_TXE) == RESET) {
            if ((spiTimeout--) == 0)
                return spiTimeoutUserCallback();
        }
        SPI_SendData(SPI_BUSE, b);
        spiTimeout = 1000;
        while (SPI_I2S_GetFlagStatus(SPI_BUSE, SPI_I2S_FLAG_RXNE) == RESET) {
            if ((spiTimeout--) == 0)
                return spiTimeoutUserCallback();
        }

        b = SPI_ReceiveData(SPI_BUSE);
        if (out)
            *(out++) = b;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
// Set SPI Divisor
///////////////////////////////////////////////////////////////////////////////

void setSPIdivisor(uint16_t divisor)
{
#define BR_CLEAR_MASK 0xFFC7

    uint16_t tempRegister;

    SPI_Cmd(SPI_BUSE, DISABLE);

    tempRegister = SPI_BUSE->CR1;

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

    SPI_BUSE->CR1 = tempRegister;

    SPI_Cmd(SPI_BUSE, ENABLE);
}

///////////////////////////////////////////////////////////////////////////////
// Get SPI Error Count
///////////////////////////////////////////////////////////////////////////////
uint32_t spiTimeoutUserCallback(void)
{
	spiErrorCount++;
    return -1;
}

uint16_t spiGetErrorCounter(void)
{
    return spiErrorCount;
}

void spiResetErrorCounter(void)
{
    spiErrorCount = 0;
}
