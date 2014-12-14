#pragma once

#define MPU6000_CS_GPIO       GPIOE
#define MPU6000_CS_PIN        GPIO_Pin_10

#define DISABLE_MPU6000       GPIO_SetBits(MPU6000_CS_GPIO,   MPU6000_CS_PIN)
#define ENABLE_MPU6000        GPIO_ResetBits(MPU6000_CS_GPIO, MPU6000_CS_PIN)

#define MPU6000_CONFIG		    	0x1A

#define BITS_DLPF_CFG_256HZ         0x00
#define BITS_DLPF_CFG_188HZ         0x01
#define BITS_DLPF_CFG_98HZ          0x02
#define BITS_DLPF_CFG_42HZ          0x03

#define GYRO_SCALE_FACTOR  0.00053292f  // (4/131) * pi/180   (32.75 LSB = 1 DPS)

bool mpu6000DetectSpi(sensor_t *acc, sensor_t *gyro, uint16_t lpf );

//bool mpu6000GyroRead(int16_t *gyroData);

//bool mpu6000AccRead(int16_t *gyroData);

bool mpu6000Read(int16_t *gyroDataG, int16_t *gyroDataA);
