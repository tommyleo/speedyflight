#include "board.h"
#include "mw.h"

#define MAG_ADDRESS 0x1E
#define MAG_DATA_REGISTER 0x03

#define HMC58X3_R_CONFA 0
#define HMC58X3_R_CONFB 1
#define HMC58X3_R_MODE 2
#define HMC58X3_X_SELF_TEST_GAUSS (+1.16f)       // X axis level when bias current is applied.
#define HMC58X3_Y_SELF_TEST_GAUSS (+1.16f)       // Y axis level when bias current is applied.
#define HMC58X3_Z_SELF_TEST_GAUSS (+1.08f)       // Z axis level when bias current is applied.
#define SELF_TEST_LOW_LIMIT  (243.0f / 390.0f)    // Low limit when gain is 5.
#define SELF_TEST_HIGH_LIMIT (575.0f / 390.0f)    // High limit when gain is 5.
#define HMC_POS_BIAS 1
#define HMC_NEG_BIAS 2

#define PositiveBiasConfig   0x11
#define NegativeBiasConfig   0x12
#define NormalOperation      0x10
#define ModeRegister         0x02
#define ContinuousConversion 0x00
#define SingleConversion     0x01

// ConfigRegA valid sample averaging for 5883L
#define SampleAveraging_1    0x00
#define SampleAveraging_2    0x01
#define SampleAveraging_4    0x02
#define SampleAveraging_8    0x03

// ConfigRegA valid data output rates for 5883L
#define DataOutputRate_0_75HZ 0x00
#define DataOutputRate_1_5HZ  0x01
#define DataOutputRate_3HZ    0x02
#define DataOutputRate_7_5HZ  0x03
#define DataOutputRate_15HZ   0x04
#define DataOutputRate_30HZ   0x05
#define DataOutputRate_75HZ   0x06

static sensor_align_e magAlign = 0;
static float magGain[3] = { 1.0f, 1.0f, 1.0f };

bool hmc5883lDetect(mag_t *mag)
//bool hmc5883lDetect(sensor_t *mag)
{
    bool ack = false;
    uint8_t sig = 0;

    ack = i2cRead(MAG_ADDRESS, 0x0A, 1, &sig);
    if (!ack || sig != 'H')
        return false;

    mag->init = hmc5883lInit;
    mag->read = hmc5883lRead;

    return true;
}

void hmc5883lInit(sensor_align_e align)
{
    gpio_config_t gpio;
    int16_t magADC[3];
    int i;
    int32_t xyz_total[3] = { 0, 0, 0 }; // 32 bit totals so they won't overflow.
    bool bret = true;           // Error indicator

    if (align > 0)
        magAlign = align;

	// PB12 - MAG_DRDY output on rev4 hardware
	gpio.pin = Pin_12;
	gpio.speed = Speed_2MHz;
	gpio.mode = Mode_IN_FLOATING;
	gpioInit(GPIOB, &gpio);

    delay(50);
    i2cWrite(MAG_ADDRESS, HMC58X3_R_CONFA, 0x010 + HMC_POS_BIAS);   // Reg A DOR = 0x010 + MS1, MS0 set to pos bias
    // Note that the  very first measurement after a gain change maintains the same gain as the previous setting.
    // The new gain setting is effective from the second measurement and on.
    i2cWrite(MAG_ADDRESS, HMC58X3_R_CONFB, 0x60); // Set the Gain to 2.5Ga (7:5->011)
    delay(100);
    hmc5883lRead(magADC);

    for (i = 0; i < 10; i++) {  // Collect 10 samples
        i2cWrite(MAG_ADDRESS, HMC58X3_R_MODE, 1);
        delay(50);
        hmc5883lRead(magADC);       // Get the raw values in case the scales have already been changed.

        // Since the measurements are noisy, they should be averaged rather than taking the max.
        xyz_total[X] += magADC[X];
        xyz_total[Y] += magADC[Y];
        xyz_total[Z] += magADC[Z];

        // Detect saturation.
        if (-4096 >= min(magADC[X], min(magADC[Y], magADC[Z]))) {
            bret = false;
            break;              // Breaks out of the for loop.  No sense in continuing if we saturated.
        }
        LED1_TOGGLE;
    }

    // Apply the negative bias. (Same gain)
    i2cWrite(MAG_ADDRESS, HMC58X3_R_CONFA, 0x010 + HMC_NEG_BIAS);   // Reg A DOR = 0x010 + MS1, MS0 set to negative bias.
    for (i = 0; i < 10; i++) {
        i2cWrite(MAG_ADDRESS, HMC58X3_R_MODE, 1);
        delay(50);
        hmc5883lRead(magADC);               // Get the raw values in case the scales have already been changed.

        // Since the measurements are noisy, they should be averaged.
        xyz_total[X] -= magADC[X];
        xyz_total[Y] -= magADC[Y];
        xyz_total[Z] -= magADC[Z];

        // Detect saturation.
        if (-4096 >= min(magADC[X], min(magADC[Y], magADC[Z]))) {
            bret = false;
            break;              // Breaks out of the for loop.  No sense in continuing if we saturated.
        }
        LED1_TOGGLE;
    }

    magGain[X] = fabsf(660.0f * HMC58X3_X_SELF_TEST_GAUSS * 2.0f * 10.0f / xyz_total[X]);
    magGain[Y] = fabsf(660.0f * HMC58X3_Y_SELF_TEST_GAUSS * 2.0f * 10.0f / xyz_total[Y]);
    magGain[Z] = fabsf(660.0f * HMC58X3_Z_SELF_TEST_GAUSS * 2.0f * 10.0f / xyz_total[Z]);

    // leave test mode
    i2cWrite(MAG_ADDRESS, HMC58X3_R_CONFA, 0x70);   // Configuration Register A  -- 0 11 100 00  num samples: 8 ; output rate: 15Hz ; normal measurement mode
    i2cWrite(MAG_ADDRESS, HMC58X3_R_CONFB, 0x20);   // Configuration Register B  -- 001 00000    configuration gain 1.3Ga
    i2cWrite(MAG_ADDRESS, HMC58X3_R_MODE, 0x00);    // Mode register             -- 000000 00    continuous Conversion Mode
    delay(100);

    if (!bret) {                // Something went wrong so get a best guess
        magGain[X] = 1.0f;
        magGain[Y] = 1.0f;
        magGain[Z] = 1.0f;
    }
}

void hmc5883lRead(int16_t *magData)
{
    uint8_t buf[6];
    int16_t mag[3];

    i2cRead(MAG_ADDRESS, MAG_DATA_REGISTER, 6, buf);
    // During calibration, magGain is 1.0, so the read returns normal non-calibrated values.
    // After calibration is done, magGain is set to calculated gain values.
    mag[X] = (int16_t)(buf[0] << 8 | buf[1]) * magGain[X];
    mag[Z] = (int16_t)(buf[2] << 8 | buf[3]) * magGain[Z];
    mag[Y] = (int16_t)(buf[4] << 8 | buf[5]) * magGain[Y];

    //debug[0] = mag[X];
    //debug[1] = mag[Z];
    //debug[2] = mag[Y];
    //debug[3] = 10;

    alignSensors(mag, magData, magAlign);
}
