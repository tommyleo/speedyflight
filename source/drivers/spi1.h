#pragma once

#define SPI_BUSE            SPI1

#define SPI_GPIO            GPIOA

#define SPI_SCK_PIN         GPIO_Pin_5
#define SPI_SCK_PIN_SOURCE  GPIO_PinSource5
#define SPI_SCK_CLK         RCC_AHB1Periph_GPIOA

#define SPI_MISO_PIN        GPIO_Pin_6
#define SPI_MISO_PIN_SOURCE GPIO_PinSource6
#define SPI_MISO_CLK        RCC_AHB1Periph_GPIOA

#define SPI_MOSI_PIN        GPIO_Pin_7
#define SPI_MOSI_PIN_SOURCE GPIO_PinSource7
#define SPI_MOSI_CLK        RCC_AHB1Periph_GPIOA


bool spiInit(void);

uint8_t spiTransferByte(uint8_t in);

uint8_t spiTransfer(uint8_t *out, uint8_t *in, int len);

void setSPIdivisor(uint16_t divisor);

uint32_t spiTimeoutUserCallback(void);

uint16_t spiGetErrorCounter(void);

void spiResetErrorCounter(void);
