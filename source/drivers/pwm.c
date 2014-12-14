#include "board.h"
#include "mw.h"

typedef struct {
    volatile uint32_t *ccr;
    uint16_t period;
    // for input only
    uint8_t channel;
    uint8_t state;
    uint16_t rise;
    uint16_t fall;
    uint16_t capture;
} pwmPortData_t;

// Pin type
enum {
    TYPE_IP = 0x10,
    TYPE_IW = 0x20,
    TYPE_M = 0x40,
    TYPE_S = 0x80
};
// Pin alias type
enum {
    TYPE_CAMSTAB1 = 1,
    TYPE_CAMSTAB2 = 2,
};
typedef struct {
    uint8_t pin;
    uint8_t af;
} pwmPintData_t;

typedef void (*pwmWriteFuncPtr)(uint8_t index, uint16_t value);  // function pointer used to write motors

static pwmPortData_t pwmPorts[MAX_PORTS];
static uint16_t captures[MAX_INPUTS];
static pwmPortData_t *motors[MAX_MOTORS];
static pwmPortData_t *servos[MAX_SERVOS];
static pwmWriteFuncPtr pwmWritePtr = NULL;
static uint8_t numMyMotors = 0;
static uint8_t numServos = 0;
static uint8_t numInputs = 0;
static uint16_t failsafeThreshold = 985;
extern int16_t failsafeCnt;


static const pwmPintData_t multiNoPWM[] = {
        { PWM9 | TYPE_M, TYPE_CAMSTAB1 },
        { PWM10 | TYPE_M, TYPE_CAMSTAB1 },
        { PWM11 | TYPE_M, 0 },
        { PWM12 | TYPE_M, 0 },
        { PWM13 | TYPE_M, TYPE_CAMSTAB2 },     // camstab
        { PWM14 | TYPE_M, TYPE_S |TYPE_CAMSTAB2 },     // camstab
        { PWM5 | TYPE_M, TYPE_S },      // or servo 1
        { PWM6 | TYPE_M, TYPE_S },      // or servo 2
        { PWM7 | TYPE_M, TYPE_S },      // or servo 3
        { PWM8 | TYPE_M, TYPE_S },      // or servo 4
        { PWM2 | TYPE_M, 0 },
        { PWM3 | TYPE_M, 0 },
        { PWM4 | TYPE_M, 0 },
        //disabled cause it use timer2 (same as potential servo)
        { PWM1 | TYPE_IP, 0 },     // PPM input , or motor output , if ppm then motor is when replaced by pwm15
        { 0xFF, 0 } };

static const pwmPintData_t multiPWM[] = {
        { PWM1 | TYPE_IW, 0 },     // input #1
        { PWM2 | TYPE_IW, 0 },
        { PWM3 | TYPE_IW, 0 },
        { PWM4 | TYPE_IW, 0 },
        { PWM5 | TYPE_IW, 0 },
        { PWM6 | TYPE_IW, 0 },
        { PWM7 | TYPE_IW, 0 },
        { PWM8 | TYPE_IW, 0 },                      // input #8
        { PWM9 | TYPE_M, TYPE_CAMSTAB1 },           // motor #1 or camstab  or motor #1
        { PWM10 | TYPE_M, TYPE_CAMSTAB1 },          // motor #2 or camstab  or motor #2
        { PWM11 | TYPE_M, 0 },                      // motor #3 or motor #3 or motor #3
        { PWM12 | TYPE_M, 0 },                      // motor #4 or motor #4 or motor #4
        { PWM13 | TYPE_M, TYPE_CAMSTAB2 },          // motor #5 or motor #1 or camstab
        { PWM14 | TYPE_M, TYPE_S | TYPE_CAMSTAB2 }, // motor #6 or servo #1 or camstab
        { 0xFF, 0 } };

static const pwmPintData_t airNoPWM[] = {
        { PWM9 | TYPE_M, 0 },      // motor #1
        { PWM10 | TYPE_M, 0 },     // motor #2
        { PWM11 | TYPE_S, 0 },
        { PWM12 | TYPE_S, 0 },
        { PWM13 | TYPE_S, 0 },
        { PWM14 | TYPE_S, 0 },
        { PWM5 | TYPE_S, TYPE_M },      // or motor 1
        { PWM6 | TYPE_S, TYPE_M },      // or motor 2
        { PWM7 | TYPE_S, TYPE_M },      // or motor 3
        { PWM8 | TYPE_S, TYPE_M },      // or motor 4
        { PWM2 | TYPE_S, 0 },
        { PWM3 | TYPE_S, TYPE_CAMSTAB2 }, // camstab
        { PWM4 | TYPE_S, TYPE_CAMSTAB2 }, // camstab
        { PWM1 | TYPE_IP, 0 },              // PPM input
        { 0xFF, 0 } };

static const pwmPintData_t airPWM[] = {
        { PWM1 | TYPE_IW, 0 },     // input #1
        { PWM2 | TYPE_IW, 0 },
        { PWM3 | TYPE_IW, 0 },
        { PWM4 | TYPE_IW, 0 },
        { PWM5 | TYPE_IW, 0 },
        { PWM6 | TYPE_IW, 0 },
        { PWM7 | TYPE_IW, 0 },
        { PWM8 | TYPE_IW, 0 },     // input #8
        { PWM9 | TYPE_S, 0 },      // servo #1
        { PWM10 | TYPE_S, 0 },
        { PWM11 | TYPE_S, 0 },
        { PWM12 | TYPE_S, 0 },
        { PWM13 | TYPE_S, TYPE_M },     // or motor 2
        { PWM14 | TYPE_M, 0 },     // motor #1
        { 0xFF, 0 } };

static const pwmPintData_t * const hardwareMaps[] = { multiPWM, multiNoPWM, airPWM, airNoPWM, };

#define PWM_TIMER_MHZ 1
#define PWM_BRUSHED_TIMER_MHZ 8

static void pwmWriteBrushed(uint8_t index, uint16_t value)
{
    *motors[index]->ccr = (value - 1000) * motors[index]->period / 1000;
}

static void pwmWriteStandard(uint8_t index, uint16_t value)
{
    *motors[index]->ccr = value;
}


void pwmWriteMotor(uint8_t index, uint16_t value)
{
    if (index < numMyMotors)
		pwmWritePtr(index, value);
}

void pwmWriteServo(uint8_t index, uint16_t value)
{
    if (index < numServos)
        *servos[index]->ccr = value;
}

uint16_t pwmRead(uint8_t channel)
{
    return captures[channel];
}

static void pwmOCConfig(TIM_TypeDef *tim, uint8_t channel, uint16_t value)
{
    TIM_OCInitTypeDef TIM_OCInitStructure;

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
    TIM_OCInitStructure.TIM_Pulse = value;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;

    switch (channel) {
        case TIM_Channel_1:
            TIM_OC1Init(tim, &TIM_OCInitStructure);
            TIM_OC1PreloadConfig(tim, TIM_OCPreload_Enable);
            break;
        case TIM_Channel_2:
            TIM_OC2Init(tim, &TIM_OCInitStructure);
            TIM_OC2PreloadConfig(tim, TIM_OCPreload_Enable);
            break;
        case TIM_Channel_3:
            TIM_OC3Init(tim, &TIM_OCInitStructure);
            TIM_OC3PreloadConfig(tim, TIM_OCPreload_Enable);
            break;
        case TIM_Channel_4:
            TIM_OC4Init(tim, &TIM_OCInitStructure);
            TIM_OC4PreloadConfig(tim, TIM_OCPreload_Enable);
            break;
    }
}

void pwmICConfig(TIM_TypeDef *tim, uint8_t channel, uint16_t polarity)
{
    TIM_ICInitTypeDef TIM_ICInitStructure;

    TIM_ICStructInit(&TIM_ICInitStructure);
    TIM_ICInitStructure.TIM_Channel = channel;
    TIM_ICInitStructure.TIM_ICPolarity = polarity;
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICInitStructure.TIM_ICFilter = 0x00;
    TIM_ICInit(tim, &TIM_ICInitStructure);
}

static void pwmGPIOConfig(GPIO_TypeDef *gpio, uint32_t pin, GPIO_Mode mode)
{
	GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(gpio, &GPIO_InitStructure);
}

static void pwmOUTGPIOConfig(GPIO_TypeDef *gpio, uint32_t pin, GPIO_Mode mode)
{
	GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(gpio, &GPIO_InitStructure);
}

static pwmPortData_t *pwmOutConfig(uint8_t port, uint8_t mhz, uint16_t period, uint16_t value)
{
    pwmPortData_t *p = &pwmPorts[port];

    TIM_Cmd(timerHardware[port].tim, DISABLE);

    configTimeBase(timerHardware[port].tim, period, mhz);
    pwmOUTGPIOConfig(timerHardware[port].gpio, timerHardware[port].pin, GPIO_Mode_AF);
    pwmOCConfig(timerHardware[port].tim, timerHardware[port].channel, value);

    // Needed only on TIM1
    if (timerHardware[port].outputEnable)
        TIM_CtrlPWMOutputs(timerHardware[port].tim, ENABLE);

    TIM_Cmd(timerHardware[port].tim, ENABLE);

    switch (timerHardware[port].channel) {
        case TIM_Channel_1:
            p->ccr = (uint32_t *)&timerHardware[port].tim->CCR1;
            break;
        case TIM_Channel_2:
            p->ccr = (uint32_t *)&timerHardware[port].tim->CCR2;
            break;
        case TIM_Channel_3:
            p->ccr = (uint32_t *)&timerHardware[port].tim->CCR3;
            break;
        case TIM_Channel_4:
            p->ccr = (uint32_t *)&timerHardware[port].tim->CCR4;
            break;
    }
    p->period = period;

    return p;
}

static pwmPortData_t *pwmInConfig(uint8_t port, timerCCCallbackPtr callback, uint8_t channel)
{
    pwmPortData_t *p = &pwmPorts[port];
    const timerHardware_t *timerHardwarePtr = &(timerHardware[port]);

    p->channel = channel;
    pwmGPIOConfig(timerHardwarePtr->gpio, timerHardwarePtr->pin, timerHardwarePtr->gpioInputMode);
    pwmICConfig(timerHardwarePtr->tim, timerHardwarePtr->channel, TIM_ICPolarity_Rising);
    timerConfigure(timerHardwarePtr, 0xFFFF, PWM_TIMER_MHZ);
    configureTimerCaptureCompareInterrupt(timerHardwarePtr, port, callback);

    return p;
}

static void failsafeCheck(uint8_t channel, uint16_t pulse)
{
    static uint8_t goodPulses;

    if (channel < 4 && pulse > failsafeThreshold)
        goodPulses |= (1 << channel);       // if signal is valid - mark channel as OK
    if (goodPulses == 0x0F) {               // If first four chanells have good pulses, clear FailSafe counter
        goodPulses = 0;
        if (failsafeCnt > 20)
            failsafeCnt -= 20;
        else
            failsafeCnt = 0;
    }
}

static void ppmCallback(uint8_t port, captureCompare_t capture)
{
    (void)port; // check for multiple ppm rx
    uint16_t diff;
    static uint16_t now;
    static uint16_t last = 0;
    static uint8_t chan = 0;

    last = now;
    now = capture;
    diff = now - last;

    if (diff > 2700) { // Per http://www.rcgroups.com/forums/showpost.php?p=21996147&postcount=3960 "So, if you use 2.5ms or higher as being the reset for the PPM stream start, you will be fine. I use 2.7ms just to be safe."
        chan = 0;
    } else {
        if (diff > PULSE_MIN && diff < PULSE_MAX && chan < MAX_INPUTS) {   // 750 to 2250 ms is our 'valid' channel range
            captures[chan] = diff;
            failsafeCheck(chan, diff);
        }
        chan++;
    }
}

static void pwmCallback(uint8_t port, captureCompare_t capture)
{
    if (pwmPorts[port].state == 0) {
        pwmPorts[port].rise = capture;
        pwmPorts[port].state = 1;
        pwmICConfig(timerHardware[port].tim, timerHardware[port].channel, TIM_ICPolarity_Falling);
    } else {
        pwmPorts[port].fall = capture;
        // compute capture
        pwmPorts[port].capture = pwmPorts[port].fall - pwmPorts[port].rise;
        if (pwmPorts[port].capture > PULSE_MIN && pwmPorts[port].capture < PULSE_MAX) { // valid pulse width
            captures[pwmPorts[port].channel] = pwmPorts[port].capture;
            failsafeCheck(pwmPorts[port].channel, pwmPorts[port].capture);
        }
        // switch state
        pwmPorts[port].state = 0;
        pwmICConfig(timerHardware[port].tim, timerHardware[port].channel, TIM_ICPolarity_Rising);
    }
}


void pwmInit(drv_pwm_config_t *config)
{
    int i = 0;
    uint8_t CamStab = TYPE_CAMSTAB2;
    const pwmPintData_t *hardwareMap;

    // enable TIMs clock
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1 | RCC_APB2Periph_TIM8, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM9, ENABLE);

	// 8 RC- Input
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource9,  GPIO_AF_TIM1);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource11, GPIO_AF_TIM1);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource13, GPIO_AF_TIM1);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource14, GPIO_AF_TIM1);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource5,  GPIO_AF_TIM9);
	GPIO_PinAFConfig(GPIOE, GPIO_PinSource6,  GPIO_AF_TIM9);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource8,  GPIO_AF_TIM8);
	GPIO_PinAFConfig(GPIOC, GPIO_PinSource9,  GPIO_AF_TIM8);

	// 6 MOTORS - Output
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_TIM2);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_TIM2);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_TIM2);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_TIM3);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource0, GPIO_AF_TIM3);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource1, GPIO_AF_TIM3);

    if (config->useTri) {
        // this is tricopter,  set camstab alias
		CamStab = TYPE_CAMSTAB1;
    }

    // to avoid importing cfg/mcfg
    failsafeThreshold = config->failsafeThreshold;

    // this is pretty hacky shit, but it will do for now. array of 4 config maps, [ multiPWM, multiNoPwm, airPWM, airNoPwm ]
    if (config->airplane)
        i = 2; // switch to air hardware config

    if (config->noPwmRx)
        i++; // next index is for non PWM reciever

    hardwareMap = hardwareMaps[i];

    for (i = 0; i < MAX_PORTS; i++) {
        uint8_t port = hardwareMap[i].pin & 0x0F;
        uint8_t mask = hardwareMap[i].pin & 0xF0;
        uint8_t afmask = hardwareMap[i].af & 0xF0;
        uint8_t afcamstab = hardwareMap[i].af & 0x0F;

        if (hardwareMap[i].pin == 0xFF) // terminator
            break;

        // if the user want the alternate function for this pin
        if (config->useAf && (afmask & (TYPE_M | TYPE_S))) {
           mask = afmask ;
        }

        if (config->useTri && (port == PWM14)) {
            // this is tricopter,  set tail servo
            mask = TYPE_S;
        }

        // Camstab demande servo pin , use the alias to find the pin
        if (config->useCamStab && (afcamstab & CamStab)) {
            mask = TYPE_S;
        }
        if (mask & TYPE_IP) {
            pwmInConfig(port, ppmCallback, 0);
            numInputs = 8;
        } else if (mask & TYPE_IW) {
			pwmInConfig(port, pwmCallback, numInputs);
			numInputs++;
        } else if (mask & TYPE_M) {
            uint32_t hz, mhz;
            if (config->motorPwmRate > 500)
                mhz = PWM_BRUSHED_TIMER_MHZ;
            else
                mhz = PWM_TIMER_MHZ;
            hz = mhz * 1000000;
			motors[numMyMotors++] = pwmOutConfig(port, mhz, hz / config->motorPwmRate, config->idlePulse);
        } else if (mask & TYPE_S) {
            servos[numServos++] = pwmOutConfig(port, PWM_TIMER_MHZ, 1000000 / config->servoPwmRate, config->servoCenterPulse);
        }
    }

    // determine motor writer function
    pwmWritePtr = pwmWriteStandard;
    if (config->motorPwmRate > 500)
        pwmWritePtr = pwmWriteBrushed;

    // set return values in init struct
    //init->numServos = numServos;
}
