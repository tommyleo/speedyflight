#pragma once

// MS5611 Defines and Variables

#define MS5611_CS_GPIO      GPIOE
#define MS5611_CS_PIN       GPIO_Pin_0

#define ENABLE_MS5611       GPIO_ResetBits(MS5611_CS_GPIO, MS5611_CS_PIN)
#define DISABLE_MS5611      GPIO_SetBits(MS5611_CS_GPIO,   MS5611_CS_PIN)

bool ms5611DetectSpi(baro_t *baro);
