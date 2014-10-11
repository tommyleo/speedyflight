#include "board.h"
#include "timer.h"

const timerHardware_t timerHardware[] = {
	{ TIM1, GPIOE, Pin_13, TIM_Channel_3, TIM1_CC_IRQn, 1, GPIO_Mode_AF},   //RC 1
	{ TIM1, GPIOE, Pin_9, TIM_Channel_1, TIM1_CC_IRQn, 1, GPIO_Mode_AF},    //   2
    { TIM1, GPIOE, Pin_11, TIM_Channel_2, TIM1_CC_IRQn, 1, GPIO_Mode_AF},   //   3
	{ TIM1, GPIOE, Pin_14, TIM_Channel_4, TIM1_CC_IRQn, 1, GPIO_Mode_AF},   //   4
    { TIM8, GPIOC, Pin_6, TIM_Channel_1, TIM8_CC_IRQn, 0, GPIO_Mode_AF},    //   5
    { TIM8, GPIOC, Pin_7, TIM_Channel_2, TIM8_CC_IRQn, 0, GPIO_Mode_AF},    //   6
    { TIM8, GPIOC, Pin_8, TIM_Channel_3, TIM8_CC_IRQn, 0, GPIO_Mode_AF},    //   7
    { TIM8, GPIOC, Pin_9, TIM_Channel_4, TIM8_CC_IRQn, 0, GPIO_Mode_AF},    //   8
    { TIM3, GPIOB, Pin_5, TIM_Channel_2, TIM3_IRQn, 0, Mode_AF_PP},         //Motors 1
    { TIM2, GPIOA, Pin_1, TIM_Channel_2, TIM2_IRQn, 0, Mode_AF_PP},         //       2
    { TIM2, GPIOA, Pin_2, TIM_Channel_3, TIM2_IRQn, 0, Mode_AF_PP},         //       3
    { TIM2, GPIOA, Pin_3, TIM_Channel_4, TIM2_IRQn, 0, Mode_AF_PP},         //       4
    { TIM3, GPIOB, Pin_0, TIM_Channel_3, TIM3_IRQn, 0, Mode_AF_PP},         //       5 TODO: check me
    { TIM3, GPIOB, Pin_1, TIM_Channel_4, TIM3_IRQn, 0, Mode_AF_PP}          //       6 TODO: check me
};

#define MAX_TIMERS 4

static const TIM_TypeDef const *timers[MAX_TIMERS] = {
    TIM1, TIM2, TIM3, TIM8};

#define CC_CHANNELS_PER_TIMER 4 // TIM_Channel_1..4
static const uint16_t channels[CC_CHANNELS_PER_TIMER] = { TIM_Channel_1, TIM_Channel_2, TIM_Channel_3, TIM_Channel_4 };

typedef struct timerConfig_s {
    TIM_TypeDef *tim;
    uint8_t channel;
    timerCCCallbackPtr *callback;
    uint8_t reference;
} timerConfig_t;

static timerConfig_t timerConfig[MAX_TIMERS * CC_CHANNELS_PER_TIMER];

static uint8_t lookupTimerIndex(const TIM_TypeDef *tim)
{
    uint8_t timerIndex = 0;
    while (timers[timerIndex] != tim) {
        timerIndex++;
    }
    return timerIndex;
}

static uint8_t lookupChannelIndex(const uint16_t channel)
{
    uint8_t channelIndex = 0;
    while (channels[channelIndex] != channel) {
        channelIndex++;
    }
    return channelIndex;
}

static uint8_t lookupTimerConfigIndex(TIM_TypeDef *tim, const uint16_t channel)
{
    return lookupTimerIndex(tim) + (MAX_TIMERS * lookupChannelIndex(channel));
}

void configureTimerChannelCallback(TIM_TypeDef *tim, uint8_t channel, uint8_t reference, timerCCCallbackPtr *callback)
{
    assert_param(IS_TIM_CHANNEL(channel));

    uint8_t timerConfigIndex = lookupTimerConfigIndex(tim, channel);

    if (timerConfigIndex >= MAX_TIMERS * CC_CHANNELS_PER_TIMER) {
        return;
    }

    timerConfig[timerConfigIndex].callback = callback;
    timerConfig[timerConfigIndex].channel = channel;
    timerConfig[timerConfigIndex].reference = reference;
}

void configureTimerInputCaptureCompareChannel(TIM_TypeDef *tim, const uint8_t channel)
{
    switch (channel) {
        case TIM_Channel_1:
            TIM_ITConfig(tim, TIM_IT_CC1, ENABLE);
            break;
        case TIM_Channel_2:
            TIM_ITConfig(tim, TIM_IT_CC2, ENABLE);
            break;
        case TIM_Channel_3:
            TIM_ITConfig(tim, TIM_IT_CC3, ENABLE);
            break;
        case TIM_Channel_4:
            TIM_ITConfig(tim, TIM_IT_CC4, ENABLE);
            break;
    }
}

void configureTimerCaptureCompareInterrupt(const timerHardware_t *timerHardwarePtr, uint8_t reference, timerCCCallbackPtr *callback)
{
    configureTimerChannelCallback(timerHardwarePtr->tim, timerHardwarePtr->channel, reference, callback);
    configureTimerInputCaptureCompareChannel(timerHardwarePtr->tim, timerHardwarePtr->channel);
}

void timerNVICConfigure(uint8_t irq)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = irq;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void configTimeBase(TIM_TypeDef *tim, uint16_t period, uint8_t mhz)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);

    if ((tim == TIM2) || (tim == TIM3)){
    	TIM_TimeBaseStructure.TIM_Period = period - 1; // 1 MHz / 400Hz = 2500 (2,5ms)
    	TIM_TimeBaseStructure.TIM_Prescaler = (uint16_t)(((SystemCoreClock / 1000000) / 2) - 1); // Shooting for 1 MHz, (1us)
	    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    }
    else{
    	TIM_TimeBaseStructure.TIM_Period = period - 1; // 1 MHz / 400Hz = 2500 (2,5ms)
    	TIM_TimeBaseStructure.TIM_Prescaler = (uint16_t)(SystemCoreClock / ((uint32_t)mhz * 1000000)) - 1;
    }
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(tim, &TIM_TimeBaseStructure);
}

void timerConfigure(const timerHardware_t *timerHardwarePtr, uint16_t period, uint8_t mhz)
{
    configTimeBase(timerHardwarePtr->tim, period, mhz);
    TIM_Cmd(timerHardwarePtr->tim, ENABLE);
    timerNVICConfigure(timerHardwarePtr->irq);
}

timerConfig_t *findTimerConfig(TIM_TypeDef *tim, uint16_t channel)
{
    uint8_t timerConfigIndex = lookupTimerConfigIndex(tim, channel);
    return &(timerConfig[timerConfigIndex]);
}

static void timCCxHandler(TIM_TypeDef *tim)
{
    captureCompare_t capture;
    timerConfig_t *tConfig;

    uint8_t channelIndex = 0;
    for (channelIndex = 0; channelIndex < CC_CHANNELS_PER_TIMER; channelIndex++) {
        uint8_t channel = channels[channelIndex];

        if (channel == TIM_Channel_1 && TIM_GetITStatus(tim, TIM_IT_CC1) == SET) {
            TIM_ClearITPendingBit(tim, TIM_IT_CC1);
            tConfig = findTimerConfig(tim, TIM_Channel_1);
            capture = TIM_GetCapture1(tim);
        } else if (channel == TIM_Channel_2 && TIM_GetITStatus(tim, TIM_IT_CC2) == SET) {
            TIM_ClearITPendingBit(tim, TIM_IT_CC2);
            tConfig = findTimerConfig(tim, TIM_Channel_2);
            capture = TIM_GetCapture2(tim);
        } else if (channel == TIM_Channel_3 && TIM_GetITStatus(tim, TIM_IT_CC3) == SET) {
            TIM_ClearITPendingBit(tim, TIM_IT_CC3);
            tConfig = findTimerConfig(tim, TIM_Channel_3);
            capture = TIM_GetCapture3(tim);
        } else if (channel == TIM_Channel_4 && TIM_GetITStatus(tim, TIM_IT_CC4) == SET) {
            TIM_ClearITPendingBit(tim, TIM_IT_CC4);
            tConfig = findTimerConfig(tim, TIM_Channel_4);
            capture = TIM_GetCapture4(tim);
        } else {
            continue; // avoid uninitialised variable dereference
        }
        if (!tConfig->callback) {
            continue;
        }
        tConfig->callback(tConfig->reference, capture);
    }
}

void TIM1_CC_IRQHandler(void)
{
    timCCxHandler(TIM1);
}

void TIM2_IRQHandler(void)
{
    timCCxHandler(TIM2);
}

void TIM3_IRQHandler(void)
{
    timCCxHandler(TIM3);
}

void TIM8_CC_IRQHandler(void)
{
    timCCxHandler(TIM8);
}
