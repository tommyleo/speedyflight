#pragma once

#define SPI_BUSE2             SPI2

#define SPI_GPIO_2            GPIOB

#define SPI_SCK_PIN_2         GPIO_Pin_13
#define SPI_SCK_PIN_SOURCE_2  GPIO_PinSource13
#define SPI_SCK_CLK_2         RCC_AHB1Periph_GPIOB

#define SPI_MISO_PIN_2        GPIO_Pin_14
#define SPI_MISO_PIN_SOURCE_2 GPIO_PinSource14
#define SPI_MISO_CLK_2        RCC_AHB1Periph_GPIOB

#define SPI_MOSI_PIN_2        GPIO_Pin_15
#define SPI_MOSI_PIN_SOURCE_2 GPIO_PinSource15
#define SPI_MOSI_CLK_2        RCC_AHB1Periph_GPIOB


void spiInit2(void);

uint8_t spiTransferByte2(uint8_t data);
uint8_t spiTransfer2(uint8_t *out, uint8_t *in, int len);

void setSPIdivisor2(uint16_t divisor);
uint32_t spiTimeoutUserCallback2(void);
uint16_t spiGetErrorCounter2(void);
void spiResetErrorCounter2(void);
