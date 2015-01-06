#include "board.h"
#include "mw.h"
#include "telemetry_common.h"

core_t core;
int hw_revision = 0;

extern rcReadRawDataPtr rcReadRawFunc;
extern uint16_t pwmReadRawRC(uint8_t chan);

int main(void)
{
	uint8_t i;
    drv_pwm_config_t pwm_params;
    drv_adc_config_t adc_params;

    RCC_HSEConfig(RCC_HSE_ON);
    while(!RCC_WaitForHSEStartUp())
    {
    }

    hw_revision = NAZE32;

	checkFirstTime(false);           // check the need to load default config
    loadAndActivateConfig();         // load master and profile configuration

	gpioStart();

	core.mainport = usbInit();      // start serial

//    // configure power ADC
//    if (mcfg.power_adc_channel > 0 && (mcfg.power_adc_channel == 1 || mcfg.power_adc_channel == 9))
//        adc_params.powerAdcChannel = mcfg.power_adc_channel;
//    else {
//        adc_params.powerAdcChannel = 0;
//        mcfg.power_adc_channel = 0;
//    }
//    adcInit(&adc_params);

    // Check battery type/voltage
//    if (feature(FEATURE_VBAT))
//        batteryInit();

    spiInit();
    spiInit2();

    if (feature(FEATURE_I2C)){
		i2cInit();
	    LED2_OFF;
	    for (i = 0; i < 20; i++) {
	        LED2_TOGGLE;
	        delay(25);
	    }
	    LED2_OFF;
    }

    initBoardAlignment();
    sensorsSet(SENSORS_SET);        // we have these sensors; SENSORS_SET defined in board.h depending on hardware platform
    sensorsAutodetect();            // drop out any sensors that don't seem to work, init all the others.

    if (!sensors(SENSOR_GYRO))      // if gyro was not detected, we give up now.
    	failureMode(3);

	// configure PWM/CPPM read function and max number of channels. spektrum or sbus below will override both of these, if enabled
	for (i = 0; i < RC_CHANS; i++)
		rcData[i] = 1502;
	rcReadRawFunc = pwmReadRawRC;
	core.numRCChannels = MAX_INPUTS;

	if (feature(FEATURE_SERIALRX)) {
		switch (mcfg.serialrx_type) {
			case SERIALRX_SPEKTRUM1024:
			case SERIALRX_SPEKTRUM2048:
				spektrumInit(&rcReadRawFunc);
				break;
			case SERIALRX_SBUS:
				sbusInit(&rcReadRawFunc);
				break;
			case SERIALRX_SUMD:
				sumdInit(&rcReadRawFunc);
				break;
			case SERIALRX_MSP:
				mspRxInit(&rcReadRawFunc);
				break;
		}
	}

	// now init other serial
	if (feature(FEATURE_TELEMETRY)) {
		initTelemetry(USART3);
	}

	if (feature(FEATURE_GPS)) {
		gpsInit(mcfg.gps_baudrate);
	}
	else{

	}

    LED0_OFF;
	LED1_ON;
    for (i = 0; i < 20; i++) {
        LED0_TOGGLE;
        delay(25);
        LED1_TOGGLE;
        delay(25);
    }
    LED0_ON;
    LED1_OFF;

    mspInit();                  // this will configure the aux box based on features and detected sensors
    imuInit();                  // set initial flight constant like gravitation or other user defined setting
    mixerInit();                // this will set core.useServo var depending on mixer type

	pwm_params.useRcUART = feature(FEATURE_GPS) && feature(FEATURE_SERIALRX) && core.telemport;
	pwm_params.useAf = feature(FEATURE_AF);
	pwm_params.noPwmRx = feature(FEATURE_PPM) || feature(FEATURE_SERIALRX);
	pwm_params.useSerialrx = feature(FEATURE_SERIALRX);
	pwm_params.useI2c = feature(FEATURE_I2C);
	pwm_params.useCamStab = feature(FEATURE_SERVO_TILT) || (mcfg.mixerConfiguration == MULTITYPE_GIMBAL);
	pwm_params.useTri = mcfg.mixerConfiguration == MULTITYPE_TRI;
	pwm_params.airplane = mcfg.mixerConfiguration == MULTITYPE_AIRPLANE;
	pwm_params.motorPwmRate = mcfg.motor_pwm_rate;
	pwm_params.servoPwmRate = mcfg.servo_pwm_rate;
	pwm_params.idlePulse = PULSE_1MS;                  // standard PWM for brushless ESC (default, overridden below)
	if (feature(FEATURE_3D))
		pwm_params.idlePulse = mcfg.neutral3d;
	if (pwm_params.motorPwmRate > 500)
		pwm_params.idlePulse = 0;                  // brushed motors
	pwm_params.servoCenterPulse = mcfg.midrc;
	pwm_params.failsafeThreshold = cfg.failsafe_detect_threshold;
	pwmInit(&pwm_params);

    previousTime = micros();

    if (mcfg.mixerConfiguration == MULTITYPE_GIMBAL)
        calibratingA = CALIBRATING_ACC_CYCLES;

    calibratingG = CALIBRATING_GYRO_CYCLES;
    calibratingB = CALIBRATING_BARO_CYCLES;             // 10 seconds init_delay + 200 * 25 ms = 15 seconds before ground pressure settles
    f.SMALL_ANGLE = 1;

    while (1) {
        loop();
    }

}

void HardFault_Handler(void)
{
	// fall out of the sky
    //writeAllMotors(mcfg.mincommand);
    LED0_OFF
    LED1_OFF
    LED2_OFF
    //BEEP_ON
    while (1)
        ; // Keep buzzer on
}
