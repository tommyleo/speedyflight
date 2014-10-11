#include "board.h"
#include "mw.h"

#define MODE_OFFSET 0
#define PUPD_OFFSET 2
#define OUTPUT_OFFSET 4

#define MODE_MASK ((1|2) << MODE_OFFSET)
#define PUPD_MASK ((1|2) << PUPD_OFFSET)
#define OUTPUT_MASK ((1|2) << OUTPUT_OFFSET)

void gpioStart(void)
{
    gpio_config_t gpio;

    // enable clock
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

    // Make all GPIO in by default to save power and reduce noise
    gpio.pin = Pin_All & ~(Pin_13 | Pin_14 | Pin_15);
    gpio.mode = Mode_AIN;
    gpioInit(GPIOA, &gpio);
    gpio.pin = Pin_All;
    gpioInit(GPIOB, &gpio);
    gpioInit(GPIOC, &gpio);
    gpioInit(GPIOD, &gpio);
    gpioInit(GPIOE, &gpio);

    struct {
        GPIO_TypeDef *gpio;
        gpio_config_t cfg;
    } gpio_setup[] = {
#ifdef LED0
            {
				.gpio = LED0_GPIO,
				.cfg = { LED0_PIN, Mode_Out_OD, Speed_2MHz } },
#endif
#ifdef LED1

            {
                .gpio = LED1_GPIO,
                .cfg = {LED1_PIN, Mode_Out_OD, Speed_2MHz}
            },
#endif
#ifdef LED2

            {
                .gpio = LED2_GPIO,
                .cfg = {LED2_PIN, Mode_Out_OD, Speed_2MHz}
            },
#endif
#ifdef BUZZER
            { .gpio = BEEP_GPIO, .cfg = { BEEP_PIN, Mode_Out_PP, Speed_2MHz } },
#endif
            };

    uint32_t i;
    uint8_t gpio_count = sizeof(gpio_setup) / sizeof(gpio_setup[0]);
    for (i = 0; i < gpio_count; i++) {
        gpioInit(gpio_setup[i].gpio, &gpio_setup[i].cfg);
    }
}

void gpioInit(GPIO_TypeDef *gpio, gpio_config_t *config)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    uint32_t pinIndex;
    for (pinIndex = 0; pinIndex < 16; pinIndex++) {
        uint32_t pinMask = (0x1 << pinIndex);
        if (config->pin & pinMask) {
            GPIO_InitStructure.GPIO_Pin = config->pin;
            GPIO_InitStructure.GPIO_Mode = config->mode;
            GPIOSpeed_TypeDef speed = GPIO_Speed_25MHz;
            switch (config->speed) {
                case Speed_10MHz:
                case Speed_25MHz:
                    speed = GPIO_Speed_25MHz;
                    break;
                case Speed_2MHz:
                    speed = GPIO_Speed_2MHz;
                    break;
                case Speed_50MHz:
                    speed = GPIO_Speed_50MHz;
                    break;
            }
            GPIO_InitStructure.GPIO_Speed = speed;
            GPIO_InitStructure.GPIO_OType = (config->mode >> OUTPUT_OFFSET) & OUTPUT_MASK;
            GPIO_InitStructure.GPIO_PuPd = (config->mode >> PUPD_OFFSET) & PUPD_MASK;
            GPIO_Init(gpio, &GPIO_InitStructure);
        }
    }
}

void gpioExtiLineConfig(uint8_t portsrc, uint8_t pinsrc)
{
// FIXME needed yet? implement?
}
