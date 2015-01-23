#pragma once

typedef uint16_t captureCompare_t;

typedef void timerCCCallbackPtr(uint8_t port, captureCompare_t capture);

typedef struct {
    TIM_TypeDef *tim;
    GPIO_TypeDef *gpio;
    uint32_t pin;
    uint8_t channel;
    uint8_t irq;
    uint8_t outputEnable;
    GPIO_Mode gpioInputMode;
} timerHardware_t;

extern const timerHardware_t timerHardware[];

void configTimeBase(TIM_TypeDef *tim, uint16_t period, uint8_t mhz);
void timerConfigure(const timerHardware_t *timerHardwarePtr, uint16_t period, uint8_t mhz);
void timerNVICConfigure(uint8_t irq);
void timerForceOverflow(TIM_TypeDef *tim);

void configureTimerInputCaptureCompareChannel(TIM_TypeDef *tim, const uint8_t channel);
void configureTimerCaptureCompareInterrupt(const timerHardware_t *timerHardwarePtr, uint8_t reference, timerCCCallbackPtr *callback);
void configureTimerChannelCallback(TIM_TypeDef *tim, uint8_t channel, uint8_t reference, timerCCCallbackPtr *callback);
