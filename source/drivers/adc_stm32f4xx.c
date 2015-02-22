#include "board.h"

///////////////////////////////////////////////////////////////////////////////
//  ADC Defines and Variables
///////////////////////////////////////////////////////////////////////////////

/*
#define VBATT_PIN              GPIO_Pin_0
#define VBATT_GPIO             GPIOC
#define VBATT_CHANNEL          ADC_Channel_10

#define ADC_PIN                GPIO_Pin_0
#define ADC_GPIO               GPIOC
#define ADC_CHANNEL            ADC_Channel_10
*/

typedef struct adc_config_t {
    uint8_t adcChannel;         // ADC1_INxx channel number
    uint8_t dmaIndex;           // index into DMA buffer in case of sparse channels
} adc_config_t;

static adc_config_t adcConfig[ADC_CHANNEL_MAX];
static volatile uint16_t adcValues[ADC_CHANNEL_MAX];
///////////////////////////////////////

uint16_t adc2ConvertedValues[8] = { 0, 0, 0, 0, 0, 0, 0, 0, };

///////////////////////////////////////////////////////////////////////////////
//  ADC Initialization
///////////////////////////////////////////////////////////////////////////////

void adcInit(drv_adc_config_t *init)
{
    int numChannels = 1, i, rank = 1;

    // configure always-present battery index (ADC4)
    //adcConfig[ADC_BATTERY].adcChannel = ADC_Channel_4;
    //adcConfig[ADC_BATTERY].dmaIndex = numChannels - 1;

}

///////////////////////////////////////////////////////////////////////////////
//  Voltage Monitor
///////////////////////////////////////////////////////////////////////////////

float voltageMonitor(void)
{
    uint8_t i;
    uint16_t convertedSum = 0;

    for (i = 0; i < 4; i++)
        convertedSum += adc2ConvertedValues[i];

    return (float)convertedSum / 4.0f;
}

///////////////////////////////////////////////////////////////////////////////
//  ADC Channel
///////////////////////////////////////////////////////////////////////////////

float adcChannel(void)
{
    uint8_t i;
    uint16_t convertedSum = 0;

    for (i = 4; i < 8; i++)
        convertedSum += adc2ConvertedValues[i];

    return (float)convertedSum / 4.0f;
}

uint16_t adcGetChannel(uint8_t channel)
{
    return adcValues[adcConfig[channel].dmaIndex];
}
