#pragma once

#include "board.h"

///////////////////////////////////////////////////////////////////////////////
// GPIO Defines
////////////////////////////////////////////////////////////////////////////////

#define digitalHi(p, i)     { p->BSRRH = i; }
#define digitalLo(p, i)     { p->BSRRL = i; }
#define digitalToggle(p, i) { p->ODR ^= i; }
#define digitalIn(p, i)     ( p->IDR & i)

#define BEEP_OFF      digitalHi(BEEP_GPIO, BEEP_PIN)
#define BEEP_ON       digitalLo(BEEP_GPIO, BEEP_PIN)
#define BEEP_TOGGLE   digitalToggle(BEEP_GPIO, BEEP_PIN)

#define LED0_OFF      digitalHi(LED0_GPIO, LED0_PIN)
#define LED0_ON       digitalLo(LED0_GPIO, LED0_PIN)
#define LED0_TOGGLE   digitalToggle(LED0_GPIO, LED0_PIN)

#define LED1_OFF      digitalHi(LED1_GPIO, LED1_PIN)
#define LED1_ON       digitalLo(LED1_GPIO, LED1_PIN)
#define LED1_TOGGLE   digitalToggle(LED1_GPIO, LED1_PIN)

#define LED2_OFF      digitalHi(LED2_GPIO, LED2_PIN)
#define LED2_ON       digitalLo(LED2_GPIO, LED2_PIN)
#define LED2_TOGGLE   digitalToggle(LED2_GPIO, LED2_PIN)

typedef enum {
    Pin_0 = 0x0001,
    Pin_1 = 0x0002,
    Pin_2 = 0x0004,
    Pin_3 = 0x0008,
    Pin_4 = 0x0010,
    Pin_5 = 0x0020,
    Pin_6 = 0x0040,
    Pin_7 = 0x0080,
    Pin_8 = 0x0100,
    Pin_9 = 0x0200,
    Pin_10 = 0x0400,
    Pin_11 = 0x0800,
    Pin_12 = 0x1000,
    Pin_13 = 0x2000,
    Pin_14 = 0x4000,
    Pin_15 = 0x8000,
    Pin_All = 0xFFFF
} GPIO_Pin;


typedef enum
{
    Mode_AIN = (GPIO_PuPd_NOPULL << 2) | GPIO_Mode_AN,
    Mode_IN_FLOATING = (GPIO_PuPd_NOPULL << 2) | GPIO_Mode_IN,
    Mode_IPD = (GPIO_PuPd_DOWN << 2) | GPIO_Mode_IN,
    Mode_IPU = (GPIO_PuPd_UP << 2) | GPIO_Mode_IN,
    Mode_Out_OD = (GPIO_OType_OD << 4) | GPIO_Mode_OUT,
    Mode_Out_PP = (GPIO_OType_PP << 4) | GPIO_Mode_OUT,
    Mode_AF_OD = (GPIO_OType_OD << 4) | GPIO_Mode_AF,
    Mode_AF_PP = (GPIO_OType_PP << 4) | GPIO_Mode_AF ,
    Mode_AF_PP_PD = (GPIO_OType_PP << 4) | (GPIO_PuPd_DOWN << 2) | GPIO_Mode_AF,
    Mode_AF_PP_PU = (GPIO_OType_PP << 4) | (GPIO_PuPd_UP << 2) | GPIO_Mode_AF
} GPIO_Mode;

typedef enum {
    Speed_10MHz = 1,
    Speed_2MHz,
    Speed_50MHz,
    Speed_25MHz
} GPIO_Speed;

typedef struct {
    uint16_t pin;
    GPIO_Mode mode;
    GPIO_Speed speed;
} gpio_config_t;


///////////////////////////////////////////////////////////////////////////////
// GPIO Initialization
///////////////////////////////////////////////////////////////////////////////

void gpioInit(GPIO_TypeDef *gpio, gpio_config_t *config);
void gpioStart(void);
///////////////////////////////////////////////////////////////////////////////
