#include "board.h"
#include "mw.h"

#include "cli.h"
#include "telemetry_common.h"

#include "buzzer.h"
#include "blackbox.h"

bool processFlight = true;
flags_t f;
int16_t debug[4];
uint8_t toggleBeep = 0;
uint32_t currentTime = 0;
uint32_t previousTime = 0;
uint16_t cycleTime = 0;         // this is the number in micro second to achieve a full loop, it can differ a little and is taken into account in the PID loop
uint32_t imuDeadline = 1000;

int16_t headFreeModeHold;

uint16_t vbat;                  // battery voltage in 0.1V steps
uint16_t vbatLatest = 0;
int32_t amperage;               // amperage read by current sensor in centiampere (1/100th A)
int32_t mAhdrawn;              // milliampere hours drawn from the battery since start
int16_t telemTemperature1;      // gyro sensor temperature

int16_t failsafeCnt = 0;
int16_t failsafeEvents = 0;
int16_t rcData[RC_CHANS];       // interval [1000;2000]
int16_t rcCommand[4];           // interval [1000;2000] for THROTTLE and [-500;+500] for ROLL/PITCH/YAW
int16_t lookupPitchRollRC[PITCH_LOOKUP_LENGTH];     // lookup table for expo & RC rate PITCH+ROLL
int16_t lookupThrottleRC[THROTTLE_LOOKUP_LENGTH];   // lookup table for expo & mid THROTTLE
uint16_t rssi;                  // range: [0;1023]
rcReadRawDataPtr rcReadRawFunc = NULL;  // receive data from default (pwm/ppm) or additional (spek/sbus/?? receiver drivers)

static void pidMultiWii(void);
static void pidRewrite(void);
static void pidLuxFloat(void);
static void pidHarakiri(void);
pidControllerFuncPtr pid_controller = pidMultiWii; // which pid controller are we using, defaultMultiWii

uint8_t dynP8[3], dynI8[3], dynD8[3];
uint8_t rcOptions[CHECKBOXITEMS];

int16_t axisPID[3];

int32_t axisPID_P[3], axisPID_I[3], axisPID_D[3];

float P_f[3];
float I_f[3];
float D_f[3];


// **********************
// GPS
// **********************
int32_t GPS_coord[2];
int32_t GPS_home[3];
int32_t GPS_hold[3];
uint8_t GPS_numSat;
uint16_t GPS_distanceToHome;        // distance to home point in meters
int16_t GPS_directionToHome;        // direction to home or hol point in degrees
uint16_t GPS_altitude, GPS_speed;   // altitude in 0.1m and speed in 0.1m/s
uint8_t GPS_update = 0;             // it's a binary toogle to distinct a GPS position update
int16_t GPS_angle[3] = { 0, 0, 0 }; // it's the angles that must be applied for GPS correction
uint16_t GPS_ground_course = 0;     // degrees * 10
int16_t nav[2];
int16_t nav_rated[2];               // Adding a rate controller to the navigation to make it smoother
int8_t nav_mode = NAV_MODE_NONE;    // Navigation mode
uint8_t GPS_numCh;                  // Number of channels
uint8_t GPS_svinfo_chn[16];         // Channel number
uint8_t GPS_svinfo_svid[16];        // Satellite ID
uint8_t GPS_svinfo_quality[16];     // Bitfield Qualtity
uint8_t GPS_svinfo_cno[16];         // Carrier to Noise Ratio (Signal Strength)

// Automatic ACC Offset Calibration
bool AccInflightCalibrationArmed = false;
bool AccInflightCalibrationMeasurementDone = false;
bool AccInflightCalibrationSavetoEEProm = false;
bool AccInflightCalibrationActive = false;
uint16_t InflightcalibratingA = 0;

// Battery monitoring stuff
uint8_t batteryCellCount = 3;       // cell count
uint16_t batteryWarningVoltage;     // slow buzzer after this one, recommended 80% of battery used. Time to land.
uint16_t batteryCriticalVoltage;    // annoying buzzer after this one, battery is going to be dead.

// Time of automatic disarm when "Don't spin the motors when armed" is enabled.
static uint32_t disarmTime = 0;

void blinkLED(uint8_t num, uint8_t wait, uint8_t repeat)
{
    uint8_t i, r;

    for (r = 0; r < repeat; r++) {
        for (i = 0; i < num; i++) {
            LED0_TOGGLE;            // switch LEDPIN state
            BEEP_ON;
            delay(wait);
            BEEP_OFF;
        }
        delay(60);
    }
}

void gpsLED(bool value){
    static uint32_t LEDTime;
	if (value){
		LED2_ON;
	}
	else{
        if ((int32_t)(currentTime - LEDTime) >= 0) {
            LEDTime = currentTime + 500000;
            LED2_TOGGLE;
        }
	}
}


void annexCode(void)
{
    static uint32_t calibratedAccTime;
    int32_t tmp, tmp2;
    int32_t axis, prop1, prop2;
    static uint8_t buzzerFreq;  // delay between buzzer ring

    // vbat shit
    static uint8_t vbatTimer = 0;
    static int32_t vbatRaw = 0;
    static int32_t amperageRaw = 0;
    static int64_t mAhdrawnRaw = 0;
    static int32_t vbatCycleTime = 0;

    // PITCH & ROLL only dynamic PID adjustemnt,  depending on throttle value
    if (rcData[THROTTLE] < cfg.tpa_breakpoint) {
        prop2 = 100;
    } else {
        if (rcData[THROTTLE] < 2000) {
            prop2 = 100 - (uint16_t)cfg.dynThrPID * (rcData[THROTTLE] - cfg.tpa_breakpoint) / (2000 - cfg.tpa_breakpoint);
        } else {
            prop2 = 100 - cfg.dynThrPID;
        }
    }

    for (axis = 0; axis < 3; axis++) {
        tmp = min(abs(rcData[axis] - mcfg.midrc), 500);
        if (axis != 2) {        // ROLL & PITCH
            if (cfg.deadband) {
                if (tmp > cfg.deadband) {
                    tmp -= cfg.deadband;
                } else {
                    tmp = 0;
                }
            }

            tmp2 = tmp / 100;
            rcCommand[axis] = lookupPitchRollRC[tmp2] + (tmp - tmp2 * 100) * (lookupPitchRollRC[tmp2 + 1] - lookupPitchRollRC[tmp2]) / 100;
            prop1 = 100 - (uint16_t)cfg.rollPitchRate * tmp / 500;
            prop1 = (uint16_t)prop1 * prop2 / 100;
        } else {                // YAW
            if (cfg.yawdeadband) {
                if (tmp > cfg.yawdeadband) {
                    tmp -= cfg.yawdeadband;
                } else {
                    tmp = 0;
                }
            }
            rcCommand[axis] = tmp * -mcfg.yaw_control_direction;
            prop1 = 100 - (uint16_t)cfg.yawRate * abs(tmp) / 500;
        }
        dynP8[axis] = (uint16_t)cfg.P8[axis] * prop1 / 100;
        dynI8[axis] = (uint16_t)cfg.I8[axis] * prop1 / 100;
        dynD8[axis] = (uint16_t)cfg.D8[axis] * prop1 / 100;
        if (rcData[axis] < mcfg.midrc)
            rcCommand[axis] = -rcCommand[axis];
    }

    tmp = constrain(rcData[THROTTLE], mcfg.mincheck, 2000);
    tmp = (uint32_t)(tmp - mcfg.mincheck) * 1000 / (2000 - mcfg.mincheck);       // [MINCHECK;2000] -> [0;1000]
    tmp2 = tmp / 100;
    rcCommand[THROTTLE] = lookupThrottleRC[tmp2] + (tmp - tmp2 * 100) * (lookupThrottleRC[tmp2 + 1] - lookupThrottleRC[tmp2]) / 100;    // [0;1000] -> expo -> [MINTHROTTLE;MAXTHROTTLE]

    if (f.HEADFREE_MODE) {
        float radDiff = (heading - headFreeModeHold) * M_PI / 180.0f;
        float cosDiff = cosf(radDiff);
        float sinDiff = sinf(radDiff);
        int16_t rcCommand_PITCH = rcCommand[PITCH] * cosDiff + rcCommand[ROLL] * sinDiff;
        rcCommand[ROLL] = rcCommand[ROLL] * cosDiff - rcCommand[PITCH] * sinDiff;
        rcCommand[PITCH] = rcCommand_PITCH;
    }

    if (feature(FEATURE_VBAT)) {
        vbatCycleTime += cycleTime;
        if (!(++vbatTimer % VBATFREQ)) {
            vbatRaw -= vbatRaw / 8;
            vbatRaw += adcGetChannel(ADC_BATTERY);
            vbat = batteryAdcToVoltage(vbatRaw / 8);

            if (mcfg.power_adc_channel > 0) {
                amperageRaw -= amperageRaw / 8;
                amperageRaw += adcGetChannel(ADC_EXTERNAL_CURRENT);
                amperage = currentSensorToCentiamps(amperageRaw / 8);
                mAhdrawnRaw += (amperage * vbatCycleTime) / 1000;
                mAhdrawn = mAhdrawnRaw / (3600 * 100);
                vbatCycleTime = 0;
            }

        }
        // Buzzers for low and critical battery levels
        if (vbat <= batteryCriticalVoltage)
            buzzer(BUZZER_BAT_CRIT_LOW);     // Critically low battery
        else if (vbat <= batteryWarningVoltage)
            buzzer(BUZZER_BAT_LOW);     // low battery
    }
    // update buzzer handler
    buzzerUpdate();

    if ((calibratingA > 0 && sensors(SENSOR_ACC)) || (calibratingG > 0)) {      // Calibration phasis
        LED0_TOGGLE;
    } else {
        if (f.ACC_CALIBRATED)
            LED0_OFF;
        if (f.ARMED)
            LED0_ON;

#ifndef CJMCU
        checkTelemetryState();
#endif
    }

#ifdef LEDRING
    if (feature(FEATURE_LED_RING)) {
        static uint32_t LEDTime;
        if ((int32_t)(currentTime - LEDTime) >= 0) {
            LEDTime = currentTime + 50000;
            ledringState();
        }
    }
#endif

    if ((int32_t)(currentTime - calibratedAccTime) >= 0) {
        if (!f.SMALL_ANGLE) {
            f.ACC_CALIBRATED = 0; // the multi uses ACC and is not calibrated or is too much inclinated
            LED0_TOGGLE;
            calibratedAccTime = currentTime + 500000;
        } else {
            f.ACC_CALIBRATED = 1;
        }
    }

    serialCom();

    if (!cliMode && feature(FEATURE_TELEMETRY)) {
        handleTelemetry();
    }

    // Read out gyro temperature. can use it for something somewhere. maybe get MCU temperature instead? lots of fun possibilities.
    //if (gyro.temperature)
    //    gyro.temperature(&telemTemperature1);
    //else {
        // TODO MCU temp
    //}
}

uint16_t pwmReadRawRC(uint8_t chan)
{
    return pwmRead(mcfg.rcmap[chan]);
}

void computeRC(void)
{
    uint16_t capture;
    int i, chan;

    if (feature(FEATURE_SERIALRX)) {
        for (chan = 0; chan < 8; chan++)
            rcData[chan] = rcReadRawFunc(chan);
    } else {
        static int16_t rcDataAverage[8][4];
        static int rcAverageIndex = 0;

        for (chan = 0; chan < 8; chan++) {
            capture = rcReadRawFunc(chan);

            // validate input
            if (capture < PULSE_MIN || capture > PULSE_MAX)
                capture = mcfg.midrc;
            rcDataAverage[chan][rcAverageIndex % 4] = capture;
            // clear this since we're not accessing it elsewhere. saves a temp var
            rcData[chan] = 0;
            for (i = 0; i < 4; i++)
                rcData[chan] += rcDataAverage[chan][i];
            rcData[chan] /= 4;
        }
        rcAverageIndex++;
    }
}

static void mwArm(void)
{
    if (calibratingG == 0 && f.ACC_CALIBRATED) {
        // TODO: feature(FEATURE_FAILSAFE) && failsafeCnt < 2
        // TODO: && ( !feature || ( feature && ( failsafecnt > 2) )
        if (!f.ARMED) {         // arm now!
            f.ARMED = 1;
            headFreeModeHold = heading;
            if (feature(FEATURE_BLACKBOX))
            	startBlackbox();
        }
    } else if (!f.ARMED) {
        blinkLED(2, 255, 1);
    }
}

static void mwDisarm(void)
{
    if (f.ARMED){
        f.ARMED = 0;
        // Reset disarm time so that it works next time we arm the board.
        if (disarmTime != 0)
        	disarmTime = 0;
        if (feature(FEATURE_BLACKBOX))
        	finishBlackbox();
    }
}

static void mwVario(void)
{

}

#define GYRO_I_MAX 256

#define RCconstPI 0.159154943092f // 0.5f / M_PI;
#define MAIN_CUT_HZ 12.0f // (default 12Hz, Range 1-50Hz)
#define OLD_YAW	0 // [0/1] 0 = multiwii 2.3 yaw, 1 = older yaw.

#define GYRO_P_MAX 300
#define GYRO_I_MAX 256

static int32_t errorGyroI[3] = { 0, 0, 0 };
static float errorGyroIf[3] = { 0.0f, 0.0f, 0.0f };
static int32_t errorAngleI[2] = { 0, 0 };
static float errorAngleIf[2] = { 0.0f, 0.0f };


// ------ Pid_Controller=0
static void pidMultiWii(void)
{
    int axis, prop;
    int32_t error, errorAngle;
    int32_t PTerm, ITerm, PTermACC = 0, ITermACC = 0, PTermGYRO = 0, ITermGYRO = 0, DTerm;
    static int16_t lastGyro[3] = { 0, 0, 0 };
    static int32_t delta1[3], delta2[3];
    int32_t deltaSum;
    int32_t delta;

    // **** PITCH & ROLL & YAW PID ****
    prop = max(abs(rcCommand[PITCH]), abs(rcCommand[ROLL])); // range [0;500]
    for (axis = 0; axis < 3; axis++) {
        if ((f.ANGLE_MODE || f.HORIZON_MODE) && axis < 2) { // MODE relying on ACC
            // 50 degrees max inclination
            errorAngle = constrain(2 * rcCommand[axis] + GPS_angle[axis], -((int)mcfg.max_angle_inclination), mcfg.max_angle_inclination) - angle[axis] + cfg.angleTrim[axis];
            PTermACC = errorAngle * cfg.P8[PIDLEVEL] / 100; // 32 bits is needed for calculation: errorAngle*P8[PIDLEVEL] could exceed 32768   16 bits is ok for result
            PTermACC = constrain(PTermACC, -cfg.D8[PIDLEVEL] * 5, +cfg.D8[PIDLEVEL] * 5);

            errorAngleI[axis] = constrain(errorAngleI[axis] + errorAngle, -10000, +10000); // WindUp
            ITermACC = (errorAngleI[axis] * cfg.I8[PIDLEVEL]) >> 12;
        }
        if (!f.ANGLE_MODE || f.HORIZON_MODE || axis == 2) { // MODE relying on GYRO or YAW axis
            error = (int32_t)rcCommand[axis] * 10 * 8 / cfg.P8[axis];
            error -= gyroData[axis];

            PTermGYRO = rcCommand[axis];

            errorGyroI[axis] = constrain(errorGyroI[axis] + error, -16000, +16000); // WindUp
            if ((abs(gyroData[axis]) > 640) || ((axis == YAW) && (abs(rcCommand[axis]) > 100)))
                errorGyroI[axis] = 0;
            ITermGYRO = (errorGyroI[axis] / 125 * cfg.I8[axis]) >> 6;
        }
        if (f.HORIZON_MODE && axis < 2) {
            PTerm = (PTermACC * (500 - prop) + PTermGYRO * prop) / 500;
            ITerm = (ITermACC * (500 - prop) + ITermGYRO * prop) / 500;
        } else {
            if (f.ANGLE_MODE && axis < 2) {
                PTerm = PTermACC;
                ITerm = ITermACC;
            } else {
                PTerm = PTermGYRO;
                ITerm = ITermGYRO;
            }
        }

        PTerm -= (int32_t)gyroData[axis] * dynP8[axis] / 10 / 8; // 32 bits is needed for calculation
        delta = gyroData[axis] - lastGyro[axis];
        lastGyro[axis] = gyroData[axis];
        deltaSum = delta1[axis] + delta2[axis] + delta;
        delta2[axis] = delta1[axis];
        delta1[axis] = delta;
        DTerm = (deltaSum * dynD8[axis]) / 32;
        axisPID[axis] = PTerm + ITerm - DTerm;

        // Values for blackbox
#ifdef BLACKBOX
        axisPID_P[axis] = PTerm;
		axisPID_I[axis] = ITerm;
		axisPID_D[axis] = -DTerm;
#endif
    }
    debug[0]=axisPID[0];
    debug[1]=axisPID[0];
    debug[2]=axisPID[0];
    debug[3]=rcCommand[3];
}

// ------ Pid_Controller=1
static void pidRewrite(void)
{
    int32_t errorAngle=0;
    int axis;
    int32_t delta, deltaSum;
    static int32_t delta1[3], delta2[3];
    int32_t PTerm, ITerm, DTerm;
    static int32_t lastError[3] = { 0, 0, 0 };
    int32_t AngleRateTmp, RateError;

    // ----------PID controller----------
    for (axis = 0; axis < 3; axis++) {
        // -----Get the desired angle rate depending on flight mode
        if (axis == 2) { // YAW is always gyro-controlled (MAG correction is applied to rcCommand)
            AngleRateTmp = (((int32_t)(cfg.yawRate + 27) * rcCommand[YAW]) >> 5);
        } else {
            // calculate error and limit the angle to max configured inclination
//#ifdef GPS
//            errorAngle = constrain(2 * rcCommand[axis] + GPS_angle[axis], -((int) max_angle_inclination),
//                    +max_angle_inclination) - inclination.raw[axis] + angleTrim->raw[axis]; // 16 bits is ok here
//#else
            errorAngle = constrain(2 * rcCommand[axis], -((int)mcfg.max_angle_inclination), mcfg.max_angle_inclination) - angle[axis] + cfg.angleTrim[axis]; // 16 bits is ok here
//#endif

//#ifdef AUTOTUNE
//            if (shouldAutotune()) {
//                errorAngle = DEGREES_TO_DECIDEGREES(autotune(rcAliasToAngleIndexMap[axis], &inclination, DECIDEGREES_TO_DEGREES(errorAngle)));
//            }
//#endif

            if (!f.ANGLE_MODE) { //control is GYRO based (ACRO and HORIZON - direct sticks control is applied to rate PID
                AngleRateTmp = ((int32_t)(cfg.rollPitchRate + 27) * rcCommand[axis]) >> 4;
                if (f.HORIZON_MODE) {
                    // mix up angle error to desired AngleRateTmp to add a little auto-level feel
                    AngleRateTmp += (errorAngle * cfg.I8[PIDLEVEL]) >> 8;
                }
            } else { // it's the ANGLE mode - control is angle based, so control loop is needed
                AngleRateTmp = (errorAngle * cfg.P8[PIDLEVEL]) >> 4;
            }
        }

        // --------low-level gyro-based PID. ----------
        // Used in stand-alone mode for ACRO, controlled by higher level regulators in other modes
        // -----calculate scaled error.AngleRates
        // multiplication of rcCommand corresponds to changing the sticks scaling here
        RateError = AngleRateTmp - gyroData[axis];

        // -----calculate P component
        PTerm = (RateError * cfg.P8[axis]) >> 7;
        // -----calculate I component
        // there should be no division before accumulating the error to integrator, because the precision would be reduced.
        // Precision is critical, as I prevents from long-time drift. Thus, 32 bits integrator is used.
        // Time correction (to avoid different I scaling for different builds based on average cycle time)
        // is normalized to cycle time = 2048.
        errorGyroI[axis] = errorGyroI[axis] + ((RateError * cycleTime) >> 11) * cfg.I8[axis];

        // limit maximum integrator value to prevent WindUp - accumulating extreme values when system is saturated.
        // I coefficient (I8) moved before integration to make limiting independent from PID settings
        errorGyroI[axis] = constrain(errorGyroI[axis], (int32_t) - GYRO_I_MAX << 13, (int32_t) + GYRO_I_MAX << 13);
        ITerm = errorGyroI[axis] >> 13;

        //-----calculate D-term
        delta = RateError - lastError[axis]; // 16 bits is ok here, the dif between 2 consecutive gyro reads is limited to 800
        lastError[axis] = RateError;

        // Correct difference by cycle time. Cycle time is jittery (can be different 2 times), so calculated difference
        // would be scaled by different dt each time. Division by dT fixes that.
        delta = (delta * ((uint16_t) 0xFFFF / (cycleTime >> 4))) >> 6;
        // add moving average here to reduce noise
        deltaSum = delta1[axis] + delta2[axis] + delta;
        delta2[axis] = delta1[axis];
        delta1[axis] = delta;
        DTerm = (deltaSum * cfg.D8[axis]) >> 8;

        // -----calculate total PID output
        axisPID[axis] = PTerm + ITerm + DTerm;

#ifdef BLACKBOX
        axisPID_P[axis] = PTerm;
        axisPID_I[axis] = ITerm;
        axisPID_D[axis] = DTerm;
#endif
    }
    debug[0]=axisPID[0];
    debug[1]=axisPID[0];
    debug[2]=axisPID[0];
    debug[3]=rcCommand[3];
}


// ------ Pid_Controller=2
static void pidLuxFloat(void)
{
    float RateError, errorAngle, AngleRate, gyroRate;
    float ITerm,PTerm,DTerm;
    int32_t stickPosAil, stickPosEle, mostDeflectedPos;
    static float lastGyroRate[3];
    static float delta1[3], delta2[3];
    float delta, deltaSum;
    float dT;
    int axis;
    float horizonLevelStrength = 1;

    float A_level = 5.0f;
    float H_level = 3.0f;
    float H_sensitivity = 75;

    /*
    P_f[ROLL] = 0.18f;     // new PID with preliminary defaults test carefully
    I_f[ROLL] = 0.038f;
    D_f[ROLL] = 0.016f;
    P_f[PITCH] = 0.21f;
    I_f[PITCH] = 0.042f;
    D_f[PITCH] = 0.021f;
    P_f[YAW] = 0.75f;
    I_f[YAW] = 0.051f;
    D_f[YAW] = 0.001f;
    */

    dT = (float)cycleTime * 0.000001f;

    /*
    if (f.HORIZON_MODE) {

        // Figure out the raw stick positions
        stickPosAil = getRcStickDeflection(ROLL, mcfg.midrc);
        stickPosEle = getRcStickDeflection(PITCH, mcfg.midrc);

        if(abs(stickPosAil) > abs(stickPosEle)){
            mostDeflectedPos = abs(stickPosAil);
        }
        else {
            mostDeflectedPos = abs(stickPosEle);
        }

        // Progressively turn off the horizon self level strength as the stick is banged over
        horizonLevelStrength = (float)(500 - mostDeflectedPos) / 500;  // 1 at centre stick, 0 = max stick deflection
        //if(pidProfile->H_sensitivity == 0){
            horizonLevelStrength = 0;
        //} else {
        //    horizonLevelStrength = constrainf(((horizonLevelStrength - 1) * (100 / pidProfile->H_sensitivity)) + 1, 0, 1);
        //}
    }
    */

    // ----------PID controller----------
    for (axis = 0; axis < 3; axis++) {

        // -----Get the desired angle rate depending on flight mode
        uint8_t rate = 0;

        if (axis == YAW) {
            // -----Get the desired angle rate depending on flight mode
            rate = cfg.yawRate;
            // YAW is always gyro-controlled (MAG correction is applied to rcCommand) 100dps to 1100dps max yaw rate
            AngleRate = (float)((rate + 10) * rcCommand[YAW]) / 50.0f;
        }
        else
        {
             // -----Get the desired angle rate depending on flight mode
             rate = cfg.rollPitchRate;

            // calculate error and limit the angle to the max inclination
//#ifdef GPS
//            errorAngle = (constrain(rcCommand[axis] + GPS_angle[axis], -((int) max_angle_inclination),
//                    +max_angle_inclination) - inclination.raw[axis] + angleTrim->raw[axis]) / 10.0f; // 16 bits is ok here
//#else
             //errorAngle = (constrain(rcCommand[axis], -((int) max_angle_inclination),
             //        +max_angle_inclination) - inclination.raw[axis] + angleTrim->raw[axis]) / 10.0f; // 16 bits is ok here
        	 errorAngle = (constrain(rcCommand[axis], -((int)mcfg.max_angle_inclination), mcfg.max_angle_inclination) - angle[axis] + cfg.angleTrim[axis]) / 10.0f; // 16 bits is ok here

//#endif

/*
#ifdef AUTOTUNE
            if (shouldAutotune()) {
                errorAngle = autotune(rcAliasToAngleIndexMap[axis], &inclination, errorAngle);
            }
#endif
*/
            if (f.ANGLE_MODE) {
                // it's the ANGLE mode - control is angle based, so control loop is needed
                AngleRate = errorAngle * A_level;
            } else {
                //control is GYRO based (ACRO and HORIZON - direct sticks control is applied to rate PID
            	AngleRate = (float)((rate + 20) * rcCommand[axis]) / 50.0f; // 200dps to 1200dps max yaw rate
/*
                if (f.HORIZON_MODE) {
                    // mix up angle error to desired AngleRate to add a little auto-level feel
                    AngleRate += errorAngle * H_level * horizonLevelStrength;
                }
*/
            }
        }

        //gyroRate = (float)gyroData[axis] * gyro.scale; // gyro output scaled to dps

        //gyroRate = (float)((float)gyroData[axis] * 4.0f) * (1.0f / 16.4f); // gyro output scaled to dps
        gyroRate = (float)((float)gyroData[axis] * 16.0f) * (1.0f / 16.4f); // gyro output scaled to dps
        //gyroRate = (float)((float)gyroData[axis] * 32.0f) * (1.0f / 16.4f); // gyro output scaled to dps


        // --------low-level gyro-based PID. ----------
        // Used in stand-alone mode for ACRO, controlled by higher level regulators in other modes
        // -----calculate scaled error.AngleRates
        // multiplication of rcCommand corresponds to changing the sticks scaling here
        RateError = AngleRate - gyroRate;

        // -----calculate P component
        PTerm = RateError * P_f[axis];
        // -----calculate I component
        errorGyroIf[axis] = constrainf(errorGyroIf[axis] + RateError * dT * I_f[axis] * 10, -250.0f, 250.0f);

        // limit maximum integrator value to prevent WindUp - accumulating extreme values when system is saturated.
        // I coefficient (I8) moved before integration to make limiting independent from PID settings
        ITerm = errorGyroIf[axis];

        //-----calculate D-term
        delta = gyroRate - lastGyroRate[axis];  // 16 bits is ok here, the dif between 2 consecutive gyro reads is limited to 800
        lastGyroRate[axis] = gyroRate;

        // Correct difference by cycle time. Cycle time is jittery (can be different 2 times), so calculated difference
        // would be scaled by different dt each time. Division by dT fixes that.
        delta *= (1.0f / dT);
        // add moving average here to reduce noise
        deltaSum = delta1[axis] + delta2[axis] + delta;
        delta2[axis] = delta1[axis];
        delta1[axis] = delta;
        DTerm = constrainf((deltaSum / 3.0f) * D_f[axis], -300.0f, 300.0f);

        // -----calculate total PID output
        axisPID[axis] = constrain(lrintf(PTerm + ITerm - DTerm), -1000, 1000);

        /*
        if (axis==0){
        	debug[0]=lrintf(PTerm);
        	debug[1]=constrain(lrintf(ITerm), -1000, 1000);
        	debug[2]=constrain(lrintf(DTerm), -1000, 1000);
        	debug[3]=axisPID[0];
        }
        */


#ifdef BLACKBOX
        axisPID_P[axis] = PTerm;
        axisPID_I[axis] = ITerm;
        axisPID_D[axis] = -DTerm;
#endif
    }
    //debug[0]=axisPID[0];
    //debug[1]=axisPID[0];
    //debug[2]=axisPID[0];
    //debug[3]=rcCommand[0];
    debug[0]=constrain(P_f[0]*1000, -1000, 1000);
    debug[1]=constrain(I_f[0]*1000, -1000, 1000);
    debug[2]=constrain(D_f[0]*1000, -1000, 1000);
    debug[3]=constrain(D_f[2]*1000, -1000, 1000);
}


// ------ Pid_Controller=5
static void pidHarakiri(void)
{
	float delta, RCfactor, rcCommandAxis, MainDptCut, gyroDataQuant;
	float PTerm, ITerm, DTerm, PTermACC = 0.0f, ITermACC = 0.0f, ITermGYRO, error, prop = 0.0f;
	static float lastGyro[2] = { 0.0f, 0.0f }, lastDTerm[2] = { 0.0f, 0.0f };
	uint8_t axis;
	float ACCDeltaTimeINS, FLOATcycleTime, Mwii3msTimescale;

	// MainDptCut = RCconstPI / (float)cfg.maincuthz; // Initialize Cut off frequencies for mainpid D
	MainDptCut = RCconstPI / MAIN_CUT_HZ; // maincuthz (default 12Hz, Range 1-50Hz), hardcoded for now
	FLOATcycleTime = (float)constrain(cycleTime, 1, 100000); // 1us - 100ms
	ACCDeltaTimeINS = FLOATcycleTime * 0.000001f; // ACCDeltaTimeINS is in seconds now
	RCfactor = ACCDeltaTimeINS / (MainDptCut + ACCDeltaTimeINS); // used for pt1 element

	if (f.HORIZON_MODE)
		prop = (float)min(max(abs(rcCommand[PITCH]), abs(rcCommand[ROLL])), 450) / 450.0f;

	for (axis = 0; axis < 2; axis++) {
		int32_t tmp = (int32_t)((float)gyroData[axis] * 0.3125f); // Multiwii masks out the last 2 bits, this has the same idea
		gyroDataQuant = (float)tmp * 3.2f; // but delivers more accuracy and also reduces jittery flight
		rcCommandAxis = (float)rcCommand[axis]; // Calculate common values for pid controllers

		if (f.ANGLE_MODE || f.HORIZON_MODE) {

#ifdef GPS
			//error = constrain(2.0f * rcCommandAxis + GPS_angle[axis], -((int) max_angle_inclination), +max_angle_inclination) - inclination.raw[axis] + angleTrim->raw[axis];
			error = constrain(2.0f * rcCommandAxis + GPS_angle[axis], -((int)mcfg.max_angle_inclination), +mcfg.max_angle_inclination) - angle[axis] + cfg.angleTrim[axis];
#else
//			error = constrain(2.0f * rcCommandAxis, -((int) max_angle_inclination), +max_angle_inclination) - inclination.raw[axis] + angleTrim->raw[axis];
			error = constrain(2.0f * rcCommandAxis, -((int)mcfg.max_angle_inclination), +mcfg.max_angle_inclination) - angle[axis] + cfg.angleTrim[axis];
#endif

//	#ifdef AUTOTUNE
//		if (shouldAutotune()) {
//			error = DEGREES_TO_DECIDEGREES(autotune(rcAliasToAngleIndexMap[axis], &inclination, DECIDEGREES_TO_DEGREES(error)));
//		}
//	#endif

			PTermACC = error * (float)cfg.P8[PIDLEVEL] * 0.008f;
			float limitf = (float)cfg.D8[PIDLEVEL] * 5.0f;
			PTermACC = constrain(PTermACC, -limitf, +limitf);
			errorAngleIf[axis] = constrain(errorAngleIf[axis] + error * ACCDeltaTimeINS, -30.0f, +30.0f);
			ITermACC = errorAngleIf[axis] * (float)cfg.I8[PIDLEVEL] * 0.08f;
		}

		if (!f.ANGLE_MODE) {
			if (abs((int16_t)gyroData[axis]) > 2560) {
				errorGyroIf[axis] = 0.0f;
			} else {
				error = (rcCommandAxis * 320.0f / (float)cfg.P8[axis]) - gyroDataQuant;
				errorGyroIf[axis] = constrainf(errorGyroIf[axis] + error * ACCDeltaTimeINS, -192.0f, +192.0f);
			}
			ITermGYRO = errorGyroIf[axis] * (float)cfg.I8[axis] * 0.01f;
			if (f.HORIZON_MODE) {
				PTerm = PTermACC + prop * (rcCommandAxis - PTermACC);
				ITerm = ITermACC + prop * (ITermGYRO - ITermACC);
			} else {
				PTerm = rcCommandAxis;
				ITerm = ITermGYRO;
			}
		}
		else{
			PTerm = PTermACC;
			ITerm = ITermACC;
		}
		PTerm -= gyroDataQuant * dynP8[axis] * 0.003f;
		delta = (gyroDataQuant - lastGyro[axis]) / ACCDeltaTimeINS;
		lastGyro[axis] = gyroDataQuant;
		lastDTerm[axis] += RCfactor * (delta - lastDTerm[axis]);
		DTerm = lastDTerm[axis] * dynD8[axis] * 0.00007f;
		axisPID[axis] = lrintf(PTerm + ITerm - DTerm); // Round up result.

#ifdef BLACKBOX
		axisPID_P[axis] = PTerm;
		axisPID_I[axis] = ITerm;
		axisPID_D[axis] = -DTerm;
#endif

	}

	Mwii3msTimescale = (int32_t)FLOATcycleTime & (int32_t)~3; // Filter last 2 bit jitter
	Mwii3msTimescale /= 3000.0f;

	if (OLD_YAW) { // [0/1] 0 = multiwii 2.3 yaw, 1 = older yaw. hardcoded for now
		PTerm = ((int32_t)cfg.P8[YAW] * (100 - (int32_t)cfg.yawRate * (int32_t)abs(rcCommand[YAW]) / 500)) / 100;
		int32_t tmp = lrintf(gyroData[YAW] * 0.25f);
		PTerm = rcCommand[YAW] - tmp * PTerm / 80;
		if ((abs(tmp) > 640) || (abs(rcCommand[YAW]) > 100)) {
			errorGyroI[YAW] = 0;
		}
		else {
			error = ((int32_t)rcCommand[YAW] * 80 / (int32_t)cfg.P8[YAW]) - tmp;
			errorGyroI[YAW] = constrain(errorGyroI[YAW] + (int32_t)(error * Mwii3msTimescale), -16000, +16000); // WindUp
			ITerm = (errorGyroI[YAW] / 125 * cfg.I8[YAW]) >> 6;
		}
	}
	else {
		int32_t tmp = ((int32_t)rcCommand[YAW] * (((int32_t)cfg.yawRate << 1) + 40)) >> 5;
		error = tmp - lrintf(gyroData[YAW] * 0.25f); // Less Gyrojitter works actually better
		if (abs(tmp) > 50) {
			errorGyroI[YAW] = 0;
		}
		else {
			errorGyroI[YAW] = constrain(errorGyroI[YAW] + (int32_t)(error * (float)cfg.I8[YAW] * Mwii3msTimescale), -268435454, +268435454);
		}
		ITerm = constrain(errorGyroI[YAW] >> 13, -GYRO_I_MAX, +GYRO_I_MAX);
		PTerm = ((int32_t)error * (int32_t)cfg.P8[YAW]) >> 6;
//		if (motorCount >= 4) { // Constrain FD_YAW by D value if not servo driven in that case servolimits apply
			int32_t limit = 300;
			if (cfg.D8[YAW]) limit -= (int32_t)cfg.D8[YAW];
				PTerm = constrain(PTerm, -limit, limit);
//		}
	}
	axisPID[YAW] = PTerm + ITerm;
	axisPID[YAW] = lrintf(axisPID[YAW]); // Round up result.

#ifdef BLACKBOX
	axisPID_P[YAW] = PTerm;
	axisPID_I[YAW] = ITerm;
	axisPID_D[YAW] = 0;
#endif

}



void setPIDController(int type)
{
    switch (type) {
        case 0:
        default:
            pid_controller = pidMultiWii;
            break;
        case 1:
            pid_controller = pidRewrite;
            break;
        case 2:
            P_f[ROLL] = (float)(cfg.P8[ROLL])/100;
            I_f[ROLL] = (float)(cfg.I8[ROLL])/1000;
            D_f[ROLL] = (float)(cfg.D8[ROLL])/1000;
            P_f[PITCH] = (float)(cfg.P8[PITCH])/100;
            I_f[PITCH] = (float)(cfg.I8[PITCH])/1000;
            D_f[PITCH] = (float)(cfg.D8[PITCH])/1000;
            P_f[YAW] = (float)(cfg.P8[YAW])/100;
            I_f[YAW] = (float)(cfg.I8[YAW])/1000;
            D_f[YAW] = (float)(cfg.D8[YAW])/1000;
            pid_controller = pidLuxFloat;
            break;
        case 5:
            pid_controller = pidHarakiri;
            break;
    }
}

void loop(void)
{
    static uint8_t rcDelayCommand;      // this indicates the number of time (multiple of RC measurement at 50Hz) the sticks must be maintained to run or switch off motors
    static uint8_t rcSticks;            // this hold sticks position for command combos
    uint8_t stTmp = 0;
    int i;
    uint16_t auxState = 0;
    bool isThrottleLow = false;
    bool rcReady = false;

    static uint32_t loopTime = 0;
    static uint32_t rcTime = 0;
    static uint32_t motorsTime = 0;

    #ifdef BARO
    static int16_t initialThrottleHold;
#endif
#ifdef GPS
    static uint8_t GPSNavReset = 1;
#endif

    // calculate rc stuff from serial-based receivers (spek/sbus)
    if (feature(FEATURE_SERIALRX)) {
        switch (mcfg.serialrx_type) {
            case SERIALRX_SPEKTRUM1024:
            case SERIALRX_SPEKTRUM2048:
                rcReady = spektrumFrameComplete();
                break;
            case SERIALRX_SBUS:
                rcReady = sbusFrameComplete();
                break;
            case SERIALRX_SUMD:
                rcReady = sumdFrameComplete();
                break;
            case SERIALRX_MSP:
                rcReady = mspFrameComplete();
                break;
        }
    }

    if (((int32_t)(currentTime - rcTime) >= 0) || rcReady) { // 50Hz or data driven

    	rcReady = false;
        rcTime = currentTime + 20000;

        computeRC();

        // in 3D mode, we need to be able to disarm by switch at any time
        if (feature(FEATURE_3D)) {
            if (!rcOptions[BOXARM])
                mwDisarm();
        }

        // Read rssi value
        //rssi = RSSI_getValue();  TODO

        // Failsafe routine
        if (feature(FEATURE_FAILSAFE)) {
            if (failsafeCnt > (5 * cfg.failsafe_delay) && f.ARMED) { // Stabilize, and set Throttle to specified level
                for (i = 0; i < 3; i++)
                    rcData[i] = mcfg.midrc;      // after specified guard time after RC signal is lost (in 0.1sec)
                rcData[THROTTLE] = cfg.failsafe_throttle;
                if (failsafeCnt > 5 * (cfg.failsafe_delay + cfg.failsafe_off_delay)) {  // Turn OFF motors after specified Time (in 0.1sec)
                    mwDisarm();             // This will prevent the copter to automatically rearm if failsafe shuts it down and prevents
                    f.OK_TO_ARM = 0;        // to restart accidentely by just reconnect to the tx - you will have to switch off first to rearm
                }
                failsafeEvents++;
            }
            if (failsafeCnt > (5 * cfg.failsafe_delay) && !f.ARMED) {  // Turn off "Ok To arm to prevent the motors from spinning after repowering the RX with low throttle and aux to arm
                mwDisarm();         // This will prevent the copter to automatically rearm if failsafe shuts it down and prevents
                f.OK_TO_ARM = 0;    // to restart accidentely by just reconnect to the tx - you will have to switch off first to rearm
            }
            failsafeCnt++;
        }
        // end of failsafe routine - next change is made with RcOptions setting

        // ------------------ STICKS COMMAND HANDLER --------------------
        // checking sticks positions
        for (i = 0; i < 4; i++) {
            stTmp >>= 2;
            if (rcData[i] > mcfg.mincheck)
                stTmp |= 0x80;  // check for MIN
            if (rcData[i] < mcfg.maxcheck)
                stTmp |= 0x40;  // check for MAX
        }
        if (stTmp == rcSticks) {
            if (rcDelayCommand < 250)
                rcDelayCommand++;
        } else
            rcDelayCommand = 0;
        rcSticks = stTmp;

        // perform actions
        if (feature(FEATURE_3D) && (rcData[THROTTLE] > (mcfg.midrc - mcfg.deadband3d_throttle) && rcData[THROTTLE] < (mcfg.midrc + mcfg.deadband3d_throttle)))
            isThrottleLow = true;
        else if (!feature(FEATURE_3D) && (rcData[THROTTLE] < mcfg.mincheck))
            isThrottleLow = true;
        if (isThrottleLow) {
            errorGyroI[ROLL] = 0;
            errorGyroI[PITCH] = 0;
            errorGyroI[YAW] = 0;
            errorAngleI[ROLL] = 0;
            errorAngleI[PITCH] = 0;
            if (cfg.activate[BOXARM] > 0) { // Arming via ARM BOX
                if (rcOptions[BOXARM] && f.OK_TO_ARM)
                    mwArm();
            }
        }

        if (cfg.activate[BOXARM] > 0) { // Disarming via ARM BOX
            if (!rcOptions[BOXARM] && f.ARMED) {
                if (mcfg.disarm_kill_switch) {
                    mwDisarm();
                } else if (isThrottleLow) {
                    mwDisarm();
                }
            }
        }

        if (rcDelayCommand == 20) {
            if (f.ARMED) {      // actions during armed
                // Disarm on throttle down + yaw
                if (cfg.activate[BOXARM] == 0 && (rcSticks == THR_LO + YAW_LO + PIT_CE + ROL_CE)){
                    mwDisarm();
                }
                // Disarm on roll (only when retarded_arm is enabled)
                if (mcfg.retarded_arm && cfg.activate[BOXARM] == 0 && (rcSticks == THR_LO + YAW_CE + PIT_CE + ROL_LO)){
                    mwDisarm();
                }
            }
            else {            // actions during not armed
                i = 0;
                // GYRO calibration
                if (rcSticks == THR_LO + YAW_LO + PIT_LO + ROL_CE) {
                    calibratingG = CALIBRATING_GYRO_CYCLES;
#ifdef GPS
                    if (feature(FEATURE_GPS))
                        GPS_reset_home_position();
#endif
                    if (sensors(SENSOR_BARO))
                        calibratingB = 10; // calibrate baro to new ground level (10 * 25 ms = ~250 ms non blocking)
                    if (!sensors(SENSOR_MAG))
                        heading = 0; // reset heading to zero after gyro calibration
                // Inflight ACC Calibration
                } else if (feature(FEATURE_INFLIGHT_ACC_CAL) && (rcSticks == THR_LO + YAW_LO + PIT_HI + ROL_HI)) {
                    if (AccInflightCalibrationMeasurementDone) {        // trigger saving into eeprom after landing
                        AccInflightCalibrationMeasurementDone = false;
                        AccInflightCalibrationSavetoEEProm = true;
                    } else {
                        AccInflightCalibrationArmed = !AccInflightCalibrationArmed;
                        if (AccInflightCalibrationArmed) {
                            toggleBeep = 2;
                        } else {
                            toggleBeep = 3;
                        }
                    }
                }

                // Multiple configuration profiles
                if (rcSticks == THR_LO + YAW_LO + PIT_CE + ROL_LO)          // ROLL left  -> Profile 1
                    i = 1;
                else if (rcSticks == THR_LO + YAW_LO + PIT_HI + ROL_CE)     // PITCH up   -> Profile 2
                    i = 2;
                else if (rcSticks == THR_LO + YAW_LO + PIT_CE + ROL_HI)     // ROLL right -> Profile 3
                    i = 3;
                if (i) {
                    mcfg.current_profile = i - 1;
                    writeEEPROM(0, false);
                    blinkLED(2, 40, i);
                    // TODO alarmArray[0] = i;
                }

                // Arm via YAW
                if (cfg.activate[BOXARM] == 0 && (rcSticks == THR_LO + YAW_HI + PIT_CE + ROL_CE))
                    mwArm();
                // Arm via ROLL
                else if (mcfg.retarded_arm && cfg.activate[BOXARM] == 0 && (rcSticks == THR_LO + YAW_CE + PIT_CE + ROL_HI))
                    mwArm();
                // Calibrating Acc
                else if (rcSticks == THR_HI + YAW_LO + PIT_LO + ROL_CE)
                    calibratingA = CALIBRATING_ACC_CYCLES;
                // Calibrating Mag
                else if (rcSticks == THR_HI + YAW_HI + PIT_LO + ROL_CE)
                    f.CALIBRATE_MAG = 1;
                i = 0;
                // Acc Trim
                if (rcSticks == THR_HI + YAW_CE + PIT_HI + ROL_CE) {
                    cfg.angleTrim[PITCH] += 2;
                    i = 1;
                } else if (rcSticks == THR_HI + YAW_CE + PIT_LO + ROL_CE) {
                    cfg.angleTrim[PITCH] -= 2;
                    i = 1;
                } else if (rcSticks == THR_HI + YAW_CE + PIT_CE + ROL_HI) {
                    cfg.angleTrim[ROLL] += 2;
                    i = 1;
                } else if (rcSticks == THR_HI + YAW_CE + PIT_CE + ROL_LO) {
                    cfg.angleTrim[ROLL] -= 2;
                    i = 1;
                }
                if (i) {
                    writeEEPROM(1, true);
                    rcDelayCommand = 0; // allow autorepetition
                }
            }
        }

        if (feature(FEATURE_INFLIGHT_ACC_CAL)) {
            if (AccInflightCalibrationArmed && f.ARMED && rcData[THROTTLE] > mcfg.mincheck && !rcOptions[BOXARM]) {   // Copter is airborne and you are turning it off via boxarm : start measurement
                InflightcalibratingA = 50;
                AccInflightCalibrationArmed = false;
            }
            if (rcOptions[BOXCALIB]) {      // Use the Calib Option to activate : Calib = TRUE Meausrement started, Land and Calib = 0 measurement stored
                if (!AccInflightCalibrationActive && !AccInflightCalibrationMeasurementDone)
                    InflightcalibratingA = 50;
                AccInflightCalibrationActive = true;
            } else if (AccInflightCalibrationMeasurementDone && !f.ARMED) {
                AccInflightCalibrationMeasurementDone = false;
                AccInflightCalibrationSavetoEEProm = true;
            }
        }

        // Check AUX switches
        for (i = 0; i < 4; i++)
            auxState |= (rcData[AUX1 + i] < 1300) << (3 * i) | (1300 < rcData[AUX1 + i] && rcData[AUX1 + i] < 1700) << (3 * i + 1) | (rcData[AUX1 + i] > 1700) << (3 * i + 2);
        for (i = 0; i < CHECKBOXITEMS; i++)
            rcOptions[i] = (auxState & cfg.activate[i]) > 0;

        // note: if FAILSAFE is disable, failsafeCnt > 5 * FAILSAVE_DELAY is always false
        if ((rcOptions[BOXANGLE] || (failsafeCnt > 5 * cfg.failsafe_delay)) && (sensors(SENSOR_ACC))) {
            // bumpless transfer to Level mode
            if (!f.ANGLE_MODE) {
                errorAngleI[ROLL] = 0;
                errorAngleI[PITCH] = 0;
                f.ANGLE_MODE = 1;
            }
        } else {
            f.ANGLE_MODE = 0;        // failsave support
        }

        if (rcOptions[BOXHORIZON]) {
            f.ANGLE_MODE = 0;
            if (!f.HORIZON_MODE) {
                errorAngleI[ROLL] = 0;
                errorAngleI[PITCH] = 0;
                f.HORIZON_MODE = 1;
            }
        } else {
            f.HORIZON_MODE = 0;
        }

        if ((rcOptions[BOXARM]) == 0)
            f.OK_TO_ARM = 1;
        if (f.ANGLE_MODE || f.HORIZON_MODE) {
            LED1_ON;
        } else {
            LED1_OFF;
        }

#ifdef BARO
        if (sensors(SENSOR_BARO)) {
            // Baro alt hold activate
            if (rcOptions[BOXBARO]) {
                if (!f.BARO_MODE) {
                    f.BARO_MODE = 1;
                    AltHold = EstAlt;
                    initialThrottleHold = rcCommand[THROTTLE];
                    errorVelocityI = 0;
                    BaroPID = 0;
                }
            } else {
                f.BARO_MODE = 0;
            }
            // Vario signalling activate
            if (feature(FEATURE_VARIO)) {
                if (rcOptions[BOXVARIO]) {
                    if (!f.VARIO_MODE) {
                        f.VARIO_MODE = 1;
                    }
                } else {
                    f.VARIO_MODE = 0;
                }
            }
        }
#endif

#ifdef  MAG
        if (sensors(SENSOR_ACC) || sensors(SENSOR_MAG)) {
            if (rcOptions[BOXMAG]) {
                if (!f.MAG_MODE) {
                    f.MAG_MODE = 1;
                    magHold = heading;
                }
            } else {
                f.MAG_MODE = 0;
            }
            if (rcOptions[BOXHEADFREE]) {
                if (!f.HEADFREE_MODE) {
                    f.HEADFREE_MODE = 1;
                }
            } else {
                f.HEADFREE_MODE = 0;
            }
            if (rcOptions[BOXHEADADJ]) {
                headFreeModeHold = heading; // acquire new heading
            }
        }
#endif

#ifdef GPS
        if (sensors(SENSOR_GPS)) {
            if (f.GPS_FIX && GPS_numSat >= 5) {
                // if both GPS_HOME & GPS_HOLD are checked => GPS_HOME is the priority
            	gpsLED(true);
                if (rcOptions[BOXGPSHOME]) {
                    if (!f.GPS_HOME_MODE) {
                        f.GPS_HOME_MODE = 1;
                        f.GPS_HOLD_MODE = 0;
                        GPSNavReset = 0;
                        GPS_set_next_wp(&GPS_home[LAT], &GPS_home[LON]);
                        nav_mode = NAV_MODE_WP;
                    }
                } else {
                    f.GPS_HOME_MODE = 0;
                    if (rcOptions[BOXGPSHOLD] && abs(rcCommand[ROLL]) < cfg.ap_mode && abs(rcCommand[PITCH]) < cfg.ap_mode) {
                        if (!f.GPS_HOLD_MODE) {
                            f.GPS_HOLD_MODE = 1;
                            GPSNavReset = 0;
                            GPS_hold[LAT] = GPS_coord[LAT];
                            GPS_hold[LON] = GPS_coord[LON];
                            GPS_set_next_wp(&GPS_hold[LAT], &GPS_hold[LON]);
                            nav_mode = NAV_MODE_POSHOLD;
                        }
                    } else {
                        f.GPS_HOLD_MODE = 0;
                        // both boxes are unselected here, nav is reset if not already done
                        if (GPSNavReset == 0) {
                            GPSNavReset = 1;
                            GPS_reset_nav();
                        }
                    }
                }
            } else {
                f.GPS_HOME_MODE = 0;
                f.GPS_HOLD_MODE = 0;
                nav_mode = NAV_MODE_NONE;
                gpsLED(false);
            }
        }
        else
        	gpsLED(false);
#else
        gpsLED(false);
#endif

        if (rcOptions[BOXPASSTHRU]) {
            f.PASSTHRU_MODE = 1;
        } else {
            f.PASSTHRU_MODE = 0;
        }

        if (mcfg.mixerConfiguration == MULTITYPE_FLYING_WING || mcfg.mixerConfiguration == MULTITYPE_AIRPLANE || mcfg.mixerConfiguration == MULTITYPE_CUSTOM_PLANE) {
            f.HEADFREE_MODE = 0;
            if (feature(FEATURE_FAILSAFE) && failsafeCnt > (6 * cfg.failsafe_delay)) {
                f.PASSTHRU_MODE = 0;
                f.ANGLE_MODE = 1;
                for (i = 0; i < 3; i++)
                    rcData[i] = mcfg.midrc;
                rcData[THROTTLE] = cfg.failsafe_throttle;
                // No GPS?  Force a soft left turn.
                if (!f.GPS_FIX && GPS_numSat <= 5) {
                    f.FW_FAILSAFE_RTH_ENABLE = 0;
                    rcData[ROLL] = mcfg.midrc - 50;
                }
            }
        }
        // When armed and motors aren't spinning. Make warning beeps so that accidentally won't lose fingers...
        // Also disarm board after 5 sec so users without buzzer won't lose fingers.
        if (feature(FEATURE_MOTOR_STOP) && f.ARMED && !f.FIXED_WING) {
            if (isThrottleLow) {
                if (disarmTime == 0)
                    disarmTime = millis() + 1000 * mcfg.auto_disarm_board;
                else if (disarmTime < millis() && mcfg.auto_disarm_board != 0)
                    mwDisarm();
                buzzer(BUZZER_ARMED);
            } else if (disarmTime != 0)
                disarmTime = 0;
        }
    }
    else { // not in rc loop
        static int taskOrder = 0;   // never call all function in the same loop, to avoid high delay spikes
        switch (taskOrder) {
        case 0:
            taskOrder++;
#ifdef MAG
            if (sensors(SENSOR_MAG) && Mag_getADC())
                break;
#endif
        case 1:
            taskOrder++;
#ifdef BARO
            if (sensors(SENSOR_BARO) && Baro_update())
                break;
#endif
        case 2:
            taskOrder++;
#ifdef BARO
            if (sensors(SENSOR_BARO) && getEstimatedAltitude())
                break;
#endif
        case 3:
            // if GPS feature is enabled, gpsThread() will be called at some intervals to check for stuck
            // hardware, wrong baud rates, init GPS if needed, etc. Don't use SENSOR_GPS here as gpsThread() can and will
            // change this based on available hardware
            taskOrder++;
#ifdef GPS
            if (feature(FEATURE_GPS)) {
                gpsThread();
                break;
            }
#endif
        case 4:
            taskOrder = 0;
#ifdef SONAR
            if (sensors(SENSOR_SONAR)) {
                Sonar_update();
            }
#endif
            if (feature(FEATURE_VARIO) && f.VARIO_MODE)
                mwVario();
            break;
        }
    }

    if (processFlight)
    {
    	processFlight=false;
		computeIMU();
		annexCode(); // non IMU critical, temeperatur, serialcom
#ifdef MAG
        if (sensors(SENSOR_MAG)) {
            if (abs(rcCommand[YAW]) < 70 && f.MAG_MODE) {
                int16_t dif = heading - magHold;
                if (dif <= -180)
                    dif += 360;
                if (dif >= +180)
                    dif -= 360;
                dif *= -mcfg.yaw_control_direction;
                if (f.SMALL_ANGLE)
                    rcCommand[YAW] -= dif * cfg.P8[PIDMAG] / 30;    // 18 deg
            } else
                magHold = heading;
        }
#endif

#ifdef BARO
        if (sensors(SENSOR_BARO)) {
            if (f.BARO_MODE) {
                static uint8_t isAltHoldChanged = 0;
                if (!f.FIXED_WING) {
                    // multirotor alt hold
                    if (cfg.alt_hold_fast_change) {
                        // rapid alt changes
                        if (abs(rcCommand[THROTTLE] - initialThrottleHold) > cfg.alt_hold_throttle_neutral) {
                            errorVelocityI = 0;
                            isAltHoldChanged = 1;
                            rcCommand[THROTTLE] += (rcCommand[THROTTLE] > initialThrottleHold) ? -cfg.alt_hold_throttle_neutral : cfg.alt_hold_throttle_neutral;
                        } else {
                            if (isAltHoldChanged) {
                                AltHold = EstAlt;
                                isAltHoldChanged = 0;
                            }
                            rcCommand[THROTTLE] = constrain(initialThrottleHold + BaroPID, mcfg.minthrottle, mcfg.maxthrottle);
                        }
                    } else {
                        // slow alt changes for apfags
                        if (abs(rcCommand[THROTTLE] - initialThrottleHold) > cfg.alt_hold_throttle_neutral) {
                            // set velocity proportional to stick movement +100 throttle gives ~ +50 cm/s
                            setVelocity = (rcCommand[THROTTLE] - initialThrottleHold) / 2;
                            velocityControl = 1;
                            isAltHoldChanged = 1;
                        } else if (isAltHoldChanged) {
                            AltHold = EstAlt;
                            velocityControl = 0;
                            isAltHoldChanged = 0;
                        }
                        rcCommand[THROTTLE] = constrain(initialThrottleHold + BaroPID, mcfg.minthrottle, mcfg.maxthrottle);
                    }
                } else {
                    // handle fixedwing-related althold. UNTESTED! and probably wrong
                    // most likely need to check changes on pitch channel and 'reset' althold similar to
                    // how throttle does it on multirotor
                	rcCommand[PITCH] += BaroPID * mcfg.fw_althold_dir;
                }
            }
        }
#endif

        if (cfg.throttle_correction_value && (f.ANGLE_MODE || f.HORIZON_MODE)) {
            rcCommand[THROTTLE] += throttleAngleCorrection;
        }

#ifdef GPS
        if (sensors(SENSOR_GPS)) {
            if ((f.GPS_HOME_MODE || f.GPS_HOLD_MODE) && f.GPS_FIX_HOME) {
                float sin_yaw_y = sinf(heading * 0.0174532925f);
                float cos_yaw_x = cosf(heading * 0.0174532925f);
                if (!f.FIXED_WING) {
                	if (cfg.nav_slew_rate) {
                        nav_rated[LON] += constrain(wrap_18000(nav[LON] - nav_rated[LON]), -cfg.nav_slew_rate, cfg.nav_slew_rate); // TODO check this on uint8
                        nav_rated[LAT] += constrain(wrap_18000(nav[LAT] - nav_rated[LAT]), -cfg.nav_slew_rate, cfg.nav_slew_rate);
                        GPS_angle[ROLL] = (nav_rated[LON] * cos_yaw_x - nav_rated[LAT] * sin_yaw_y) / 10;
                        GPS_angle[PITCH] = (nav_rated[LON] * sin_yaw_y + nav_rated[LAT] * cos_yaw_x) / 10;
                    } else {
                        GPS_angle[ROLL] = (nav[LON] * cos_yaw_x - nav[LAT] * sin_yaw_y) / 10;
                        GPS_angle[PITCH] = (nav[LON] * sin_yaw_y + nav[LAT] * cos_yaw_x) / 10;
                	}
                }//else fw_nav();
            } else {
            	GPS_angle[ROLL] = 0;
            	GPS_angle[PITCH] = 0;
            	GPS_angle[YAW] = 0;
            }
        }
#endif
	}

    currentTime = micros();
    if (mcfg.looptime == 0 || (int32_t)(currentTime - loopTime) >= 0) {
        loopTime = currentTime + mcfg.looptime;
        currentTime = micros(); // Measure loop rate just afer reading the sensors
        cycleTime = (int32_t)(currentTime - previousTime);
        previousTime = currentTime;
        processFlight=true;

        if (((int32_t)(currentTime - motorsTime) >= 0) || feature(FEATURE_SYNCPWM)) {
        	motorsTime = currentTime + (uint16_t)(1000000/mcfg.motor_pwm_rate);
			pid_controller();
			mixTable();
			//writeServos();
			writeMotors();
	        if (feature(FEATURE_BLACKBOX))
	        	handleBlackbox();
        }

    }
}
