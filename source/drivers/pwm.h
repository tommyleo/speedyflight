#pragma once

// TODO fixme , msp has only 8 motors
#define MAX_MOTORS  12
#define MAX_SERVOS  8
#define MAX_INPUTS  8
#define PULSE_1MS   (1000)      // 1ms pulse width
#define PULSE_MIN   (750)       // minimum PWM pulse width which is considered valid
#define PULSE_MAX   (2250)      // maximum PWM pulse width which is considered valid

typedef struct drv_pwm_config_t {
    bool useI2c;   // was used for serialrx on previous hardware
    bool noPwmRx;          // activate the 4 additional channels RC 5,6,7,8
    bool useSerialrx;
    bool useRcUART;
    bool useAf;
    bool useCamStab;
    bool useTri;
    bool airplane; // fixed wing hardware config, lots of servos etc
    uint8_t adcChannel;  // steal one RC input for current sensor
    uint16_t motorPwmRate;
    uint16_t servoPwmRate;
    uint16_t idlePulse;  // PWM value to use when initializing the driver. set this to either PULSE_1MS (regular pwm),
                         // some higher value (used by 3d mode), or 0, for brushed pwm drivers.
    uint16_t servoCenterPulse;
    uint16_t failsafeThreshold;
} drv_pwm_config_t;

// This indexes into the read-only hardware definition structure in pwm.c,
// as well as into pwmPorts[] structure with dynamic data.
enum {
    PWM1 = 0,
    PWM2,
    PWM3,
    PWM4,
    PWM5,
    PWM6,
    PWM7,
    PWM8,
    PWM9,
    PWM10,
    PWM11,
    PWM12,
    PWM13,
    PWM14,
    MAX_PORTS
};

void pwmInit(drv_pwm_config_t *init);
void pwmWriteMotor(uint8_t index, uint16_t value);
void pwmFinishedWritingMotors(uint8_t numberMotors);
void pwmWriteServo(uint8_t index, uint16_t value);
uint16_t pwmRead(uint8_t channel);
