#include "board.h"

///////////////////////////////////////////////////////////////////////////////
//  ADC Defines and Variables
///////////////////////////////////////////////////////////////////////////////

#define VBATT_PIN              GPIO_Pin_0
#define VBATT_GPIO             GPIOC
#define VBATT_CHANNEL          ADC_Channel_10

#define ADC_PIN                GPIO_Pin_0
#define ADC_GPIO               GPIOC
#define ADC_CHANNEL            ADC_Channel_10

///////////////////////////////////////

uint16_t adc2ConvertedValues[8] = { 0, 0, 0, 0, 0, 0, 0, 0, };

///////////////////////////////////////////////////////////////////////////////
//  ADC Initialization
///////////////////////////////////////////////////////////////////////////////

void adcInit(drv_adc_config_t *init)
{

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

///////////////////////////////////////////////////////////////////////////////
