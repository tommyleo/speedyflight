#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#include "printf.h"
#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_exti.h"
#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include "usbd_cdc_vcp.h"
#include "usb_dcd_int.h"

//Tommy aggiunte
#include "misc.h"
#include "stm32f4xx_flash.h"
#include "stm32f4xx_spi.h"
#include "stm32f4xx_i2c.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_dma.h"

/*
 * The USB data must be 4 byte aligned if DMA is enabled. This macro handles
 * the alignment, if necessary (it's actually magic, but don't tell anyone).
 */
__ALIGN_BEGIN USB_OTG_CORE_HANDLE  USB_OTG_dev __ALIGN_END;


#define SENSORS_SET (SENSOR_GYRO | SENSOR_ACC | SENSOR_BARO | SENSOR_MAG)
#define GYRO
#define ACC
#define MAG
#define BARO
#define GPS
#define LED0
#define LED1
#define LED2
//#define LEDRING
//#define SONAR
//#define BUZZER

// 96-bit Chip Unique ID on st F103/F303
#define U_ID_0 (*(uint32_t*)0x1FFFF7E8)
#define U_ID_1 (*(uint32_t*)0x1FFFF7EC)
#define U_ID_2 (*(uint32_t*)0x1FFFF7F0)

typedef enum {
    SENSOR_GYRO = 1 << 0, // always present
    SENSOR_ACC = 1 << 1,
    SENSOR_BARO = 1 << 2,
    SENSOR_MAG = 1 << 3,
    SENSOR_SONAR = 1 << 4,
    SENSOR_GPS = 1 << 5,
    SENSOR_GPSMAG = 1 << 6,
} AvailableSensors;

// Type of accelerometer used/detected
typedef enum AccelSensors {
    ACC_DEFAULT = 0,
    ACC_ADXL345 = 1,
    ACC_MPU6050 = 2,
    ACC_MMA8452 = 3,
    ACC_BMA280 = 4,
    ACC_NONE = 5
} AccelSensors;

typedef enum {
    FEATURE_PPM = 1 << 0,
    FEATURE_VBAT = 1 << 1,
    FEATURE_INFLIGHT_ACC_CAL = 1 << 2,
    FEATURE_SERIALRX = 1 << 3,
    FEATURE_MOTOR_STOP = 1 << 4,
    FEATURE_SERVO_TILT = 1 << 5,
    FEATURE_SOFTSERIAL = 1 << 6,
    FEATURE_LED_RING = 1 << 7,
    FEATURE_GPS = 1 << 8,
    FEATURE_FAILSAFE = 1 << 9,
    FEATURE_SONAR = 1 << 10,
    FEATURE_TELEMETRY = 1 << 11,
    FEATURE_POWERMETER = 1 << 12,
    FEATURE_VARIO = 1 << 13,
    FEATURE_3D = 1 << 14,
    FEATURE_I2C = 1 << 15,
    FEATURE_AF = 1 << 16,
} AvailableFeatures;

typedef enum {
    SERIALRX_SPEKTRUM1024 = 0,
    SERIALRX_SPEKTRUM2048 = 1,
    SERIALRX_SBUS = 2,
    SERIALRX_SUMD = 3,
    SERIALRX_MSP = 4,
    SERIALRX_PROVIDER_MAX = SERIALRX_MSP,
} SerialRXType;

typedef enum {
    GPS_NMEA = 0,
    GPS_UBLOX,
    GPS_MTK_NMEA,
    GPS_MTK_BINARY,
    GPS_MAG_BINARY,
    GPS_HARDWARE_MAX = GPS_MAG_BINARY,
} GPSHardware;

typedef enum {
    GPS_BAUD_115200 = 0,
    GPS_BAUD_57600,
    GPS_BAUD_38400,
    GPS_BAUD_19200,
    GPS_BAUD_9600,
    GPS_BAUD_MAX = GPS_BAUD_9600
} GPSBaudRates;

typedef enum {
    TELEMETRY_PROVIDER_MSP = 0,
    TELEMETRY_PROVIDER_FRSKY,
    TELEMETRY_PROVIDER_HOTT,
    TELEMETRY_PROVIDER_MAX = TELEMETRY_PROVIDER_HOTT
} TelemetryProvider;

typedef enum {
    TELEMETRY_PORT_UART = 0,
    TELEMETRY_PORT_SOFTSERIAL_1, // Requires FEATURE_SOFTSERIAL
    TELEMETRY_PORT_SOFTSERIAL_2, // Requires FEATURE_SOFTSERIAL
    TELEMETRY_PORT_MAX = TELEMETRY_PORT_SOFTSERIAL_2
} TelemetryPort;

typedef enum {
    X = 0,
    Y,
    Z
} sensor_axis_e;

typedef enum {
    ALIGN_DEFAULT = 0,                                      // driver-provided alignment
    CW0_DEG = 1,
    CW90_DEG = 2,
    CW180_DEG = 3,
    CW270_DEG = 4,
    CW0_DEG_FLIP = 5,
    CW90_DEG_FLIP = 6,
    CW180_DEG_FLIP = 7,
    CW270_DEG_FLIP = 8
} sensor_align_e;

enum {
    GYRO_UPDATED = 1 << 0,
    ACC_UPDATED = 1 << 1,
    MAG_UPDATED = 1 << 2,
    TEMP_UPDATED = 1 << 3
};

typedef struct sensor_data_t {
    int16_t gyro[3];
    int16_t acc[3];
    int16_t mag[3];
    float temperature;
    int updated;
} sensor_data_t;

typedef void (*sensorInitFuncPtr)(sensor_align_e align);   // sensor init prototype
typedef bool (*sensorReadFuncPtr)(int16_t *data);          // sensor read and align prototype
typedef void (*baroOpFuncPtr)(void);                       // baro start operation
typedef void (*baroCalculateFuncPtr)(int32_t *pressure, int32_t *temperature);             // baro calculation (filled params are pressure and temperature)

typedef uint16_t (*rcReadRawDataPtr)(uint8_t chan);        // used by receiver driver to return channel data
typedef void (*pidControllerFuncPtr)(void);                // pid controller function prototype

typedef struct sensor_t {
    sensorInitFuncPtr init;                                 // initialize function
    sensorReadFuncPtr read;                                 // read 3 axis data function
    sensorReadFuncPtr temperature;                          // read temperature if available
    float scale;                                            // scalefactor (currently used for gyro only, todo for accel)
} sensor_t;

typedef struct baro_t {
    uint16_t ut_delay;
    uint16_t up_delay;
    baroOpFuncPtr start_ut;
    baroOpFuncPtr get_ut;
    baroOpFuncPtr start_up;
    baroOpFuncPtr get_up;
    baroCalculateFuncPtr calculate;
} baro_t;


#define BEEP_PIN    GPIO_Pin_4
#define BEEP_GPIO   GPIOA

#define LED0_PIN    GPIO_Pin_13  //Green
#define LED0_GPIO   GPIOC

#define LED1_PIN    GPIO_Pin_14  //Yellow
#define LED1_GPIO   GPIOC

#define LED2_PIN    GPIO_Pin_15  //Red
#define LED2_GPIO   GPIOC

#define UART1_TX_PIN        GPIO_Pin_9
#define UART1_RX_PIN        GPIO_Pin_10
#define UART1_TX_GPIO       GPIOA
#define UART1_RX_GPIO       GPIOA
#define UART1_TX_PINSOURCE  GPIO_PinSource9
#define UART1_RX_PINSOURCE  GPIO_PinSource10

#define UART2_TX_PIN        GPIO_Pin_5
#define UART2_RX_PIN        GPIO_Pin_6
#define UART2_TX_GPIO       GPIOD
#define UART2_RX_GPIO       GPIOD
#define UART2_TX_PINSOURCE  GPIO_PinSource5
#define UART2_RX_PINSOURCE  GPIO_PinSource6

#define UART3_TX_PIN        GPIO_Pin_8
#define UART3_RX_PIN        GPIO_Pin_9
#define UART3_TX_GPIO       GPIOD
#define UART3_RX_GPIO       GPIOD
#define UART3_TX_PINSOURCE  GPIO_PinSource8
#define UART3_RX_PINSOURCE  GPIO_PinSource9

#include "drivers/adc.h"
#include "drivers/crc.h"
#include "drivers/gpio.h"
#include "drivers/i2c.h"
#include "drivers/pwm.h"
#include "drivers/serial.h"
#include "drivers/softserial.h"
#include "drivers/spi1.h"
#include "drivers/spi2.h"
#include "drivers/system.h"
#include "drivers/timer.h"
#include "drivers/uart.h"
#include "drivers/usb.h"

#include "sensors.h"
#include "sensors/i2c_hmc5883l.h"
#include "sensors/i2c_ledring.h"
#include "sensors/spi_mpu6000.h"
#include "sensors/spi_ms5611.h"
#include "sensors/spi_eeprom.h"
