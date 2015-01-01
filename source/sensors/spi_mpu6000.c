#include "board.h"
#include "mw.h"
#include "spi_mpu6000.h"

// Registers
#define MPU6000_SMPLRT_DIV	    	0x19
#define MPU6000_GYRO_CONFIG	    	0x1B
#define MPU6000_ACCEL_CONFIG  		0x1C
#define MPU6000_FIFO_EN		    	0x23
#define MPU6000_INT_PIN_CFG	    	0x37
#define MPU6000_INT_ENABLE	    	0x38
#define MPU6000_INT_STATUS	    	0x3A
#define MPU6000_ACCEL_XOUT_H 		0x3B
#define MPU6000_ACCEL_XOUT_L 		0x3C
#define MPU6000_ACCEL_YOUT_H 		0x3D
#define MPU6000_ACCEL_YOUT_L 		0x3E
#define MPU6000_ACCEL_ZOUT_H 		0x3F
#define MPU6000_ACCEL_ZOUT_L    	0x40
#define MPU6000_TEMP_OUT_H	    	0x41
#define MPU6000_TEMP_OUT_L	    	0x42
#define MPU6000_GYRO_XOUT_H	    	0x43
#define MPU6000_GYRO_XOUT_L	    	0x44
#define MPU6000_GYRO_YOUT_H	    	0x45
#define MPU6000_GYRO_YOUT_L	     	0x46
#define MPU6000_GYRO_ZOUT_H	    	0x47
#define MPU6000_GYRO_ZOUT_L	    	0x48
#define MPU6000_USER_CTRL	    	0x6A
#define MPU6000_SIGNAL_PATH_RESET   0x68
#define MPU6000_PWR_MGMT_1	    	0x6B
#define MPU6000_PWR_MGMT_2	    	0x6C
#define MPU6000_FIFO_COUNTH	    	0x72
#define MPU6000_FIFO_COUNTL	    	0x73
#define MPU6000_FIFO_R_W		   	0x74
#define MPU6000_WHOAMI		    	0x75

// Bits
#define BIT_SLEEP				    0x40
#define BIT_H_RESET				    0x80
#define BITS_CLKSEL				    0x07
#define MPU_CLK_SEL_PLLGYROX	    0x01
#define MPU_CLK_SEL_PLLGYROZ	    0x03
#define MPU_EXT_SYNC_GYROX		    0x02
#define BITS_FS_250DPS              0x00
#define BITS_FS_500DPS              0x08
#define BITS_FS_1000DPS             0x10
#define BITS_FS_2000DPS             0x18
#define BITS_FS_2G                  0x00
#define BITS_FS_4G                  0x08
#define BITS_FS_8G                  0x10
#define BITS_FS_16G                 0x18
#define BITS_FS_MASK                0x18
#define BITS_DLPF_CFG_256HZ         0x00
#define BITS_DLPF_CFG_188HZ         0x01
#define BITS_DLPF_CFG_98HZ          0x02
#define BITS_DLPF_CFG_42HZ          0x03
#define BITS_DLPF_CFG_20HZ          0x04
#define BITS_DLPF_CFG_10HZ          0x05
#define BITS_DLPF_CFG_5HZ           0x06
#define BITS_DLPF_CFG_2100HZ_NOLPF  0x07
#define BITS_DLPF_CFG_MASK          0x07
#define BIT_INT_ANYRD_2CLEAR        0x10
#define BIT_RAW_RDY_EN			    0x01
#define BIT_RAW_RDY_INT             0x01
#define BIT_I2C_IF_DIS              0x10
#define BIT_INT_STATUS_DATA		    0x01
#define BIT_GYRO                    3
#define BIT_ACC                     2
#define BIT_TEMP                    1

enum gyro_fsr_e {
    INV_FSR_250DPS = 0,
    INV_FSR_500DPS,
    INV_FSR_1000DPS,
    INV_FSR_2000DPS,
    NUM_GYRO_FSR
};

enum clock_sel_e {
    INV_CLK_INTERNAL = 0,
    INV_CLK_PLL,
    NUM_CLK
};

enum accel_fsr_e {
    INV_FSR_2G = 0,
    INV_FSR_4G,
    INV_FSR_8G,
    INV_FSR_16G,
    NUM_ACCEL_FSR
};

static sensor_align_e gyroAlign = 0;
static sensor_align_e accAlign = 0;

void mpu6000GyroInit(sensor_align_e align)
{
    if (align > 0)
        gyroAlign = align;
}

void mpu6000AccInit(sensor_align_e align)
{
    acc_1G = 512 * 8;
    if (align > 0)
        accAlign = align;
}

bool mpu6000DetectSpi(sensor_t *sensorAcc, sensor_t *sensorGyro, uint16_t lpf )
{
    uint8_t mpuLowPassFilter = BITS_DLPF_CFG_42HZ;
    int16_t data[3];
    spiResetErrorCounter2();

    // default lpf is 42Hz
    switch (lpf) {
        case 256:
            mpuLowPassFilter = BITS_DLPF_CFG_256HZ;
            break;
        case 188:
            mpuLowPassFilter = BITS_DLPF_CFG_188HZ;
            break;
        case 98:
            mpuLowPassFilter = BITS_DLPF_CFG_98HZ;
            break;
        default:
        case 42:
            mpuLowPassFilter = BITS_DLPF_CFG_42HZ;
            break;
        case 20:
            mpuLowPassFilter = BITS_DLPF_CFG_20HZ;
            break;
        case 10:
            mpuLowPassFilter = BITS_DLPF_CFG_10HZ;
            break;
        case 5:
            mpuLowPassFilter = BITS_DLPF_CFG_5HZ;
        case 0:
            mpuLowPassFilter = BITS_DLPF_CFG_2100HZ_NOLPF;
            break;
    }

    ENABLE_MPU6000;
    spiTransferByte2(MPU6000_PWR_MGMT_1); // Device Reset
    spiTransferByte2(BIT_H_RESET);
    DISABLE_MPU6000;
    delay(150);

    ENABLE_MPU6000;
    spiTransferByte2(MPU6000_SIGNAL_PATH_RESET); // Device Reset
    spiTransferByte2(BIT_GYRO | BIT_ACC | BIT_TEMP);
    DISABLE_MPU6000;
    delay(150);

    ENABLE_MPU6000;
    spiTransferByte2(MPU6000_PWR_MGMT_1); // Clock Source PPL with Z axis gyro reference
    spiTransferByte2(MPU_CLK_SEL_PLLGYROZ);
    DISABLE_MPU6000;
    delayMicroseconds(10);

    ENABLE_MPU6000;
    spiTransferByte2(MPU6000_PWR_MGMT_2);
    spiTransferByte2(0x00);
    DISABLE_MPU6000;
    delayMicroseconds(1);

    ENABLE_MPU6000;
    spiTransferByte2(MPU6000_USER_CTRL);           // Disable Primary I2C Interface
    spiTransferByte2(BIT_I2C_IF_DIS);
    DISABLE_MPU6000;
    delayMicroseconds(1);

    ENABLE_MPU6000;
    spiTransferByte2(MPU6000_ACCEL_CONFIG);        // Accel +/- 8 G Full Scale
    spiTransferByte2(BITS_FS_8G);
    DISABLE_MPU6000;
    delayMicroseconds(1);

    ENABLE_MPU6000;
    spiTransferByte2(MPU6000_GYRO_CONFIG);         // Gyro +/- 2000 DPS Full Scale
    spiTransferByte2(BITS_FS_2000DPS);
    DISABLE_MPU6000;
    delayMicroseconds(1);

    ENABLE_MPU6000;
    spiTransferByte2(MPU6000_CONFIG);
    spiTransferByte2(mpuLowPassFilter);
    DISABLE_MPU6000;
    delayMicroseconds(1);

    ENABLE_MPU6000;
    spiTransferByte2(MPU6000_SMPLRT_DIV);          // Accel Sample Rate 1kHz
    spiTransferByte2(0x00);                        // Gyroscope Output Rate =  1kHz when the DLPF is enabled
    DISABLE_MPU6000;
    delayMicroseconds(1);

    setSPIdivisor2(2);

    sensorAcc->init = mpu6000AccInit;
    sensorAcc->read = NULL; //Not used
    sensorGyro->init = mpu6000GyroInit;
    sensorGyro->read = mpu6000Read;
    sensorGyro->scale = (4.0f / 16.4f) * (M_PI / 180.0f) * 0.000001f;

    return true;
}


bool mpu6000Read(int16_t *datag, int16_t *dataa)
{
    int16_t dataBuffer[3];
    uint8_t buf[14];
    uint8_t Read_OK;

    ENABLE_MPU6000;
    buf[0]=spiTransferByte2(MPU6000_INT_STATUS | 0x80);
    buf[1]=spiTransferByte2(0xFF);
    DISABLE_MPU6000;

    delayMicroseconds(5);

    debug[0] = buf[1];
    debug[1] = buf[1] & BIT_RAW_RDY_INT;

    if ((buf[1] & BIT_RAW_RDY_INT) == 0){
    	debug[2]=99;
    	return false;
    }

    ENABLE_MPU6000;
    spiTransferByte2(MPU6000_ACCEL_XOUT_H | 0x80);
    Read_OK = spiTransfer2(buf, NULL, 14);
    DISABLE_MPU6000;

    //Gyro
    if (Read_OK==1){
		dataBuffer[X] = (int16_t)((buf[8] << 8) | buf[9]) / 4;
		dataBuffer[Y] = (int16_t)((buf[10] << 8) | buf[11]) / 4;
		dataBuffer[Z] = (int16_t)((buf[12] << 8) | buf[13]) / 4;
		alignSensors(dataBuffer, datag, gyroAlign);

		//Acc
		dataBuffer[X] = (int16_t)((buf[0] << 8) | buf[1]);
		dataBuffer[Y] = (int16_t)((buf[2] << 8) | buf[3]);
		dataBuffer[Z] = (int16_t)((buf[4] << 8) | buf[5]);
		alignSensors(dataBuffer, dataa, accAlign);
	    return true;
    }
    else
    	return false;
}
