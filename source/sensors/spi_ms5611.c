#include "board.h"

typedef union {
    uint16_t value;
    uint8_t bytes[2];
} uint16andUint8_t;

typedef union {
    uint32_t value;
    uint8_t bytes[4];
} uint32andUint8_t;

//#define OSR  256  // 0.60 mSec conversion time (1666.67 Hz)
//#define OSR  512  // 1.17 mSec conversion time ( 854.70 Hz)
//#define OSR 1024  // 2.28 mSec conversion time ( 357.14 Hz)
//#define OSR 2048  // 4.54 mSec conversion time ( 220.26 Hz)
#define OSR 4096  // 9.04 mSec conversion time ( 110.62 Hz)


uint16andUint8_t c1, c2, c3, c4, c5, c6;

uint32andUint8_t d1, d2;

int32_t dT;

int32_t ms5611Temperature;

///////////////////////////////////////////////////////////////////////////////
// Calculate Temperature
///////////////////////////////////////////////////////////////////////////////

int32_t calculateTemperature(void)
{
    dT = (int32_t)d2.value - ((int32_t)c5.value << 8);
    ms5611Temperature = 2000 + (int32_t)(((int64_t)dT * c6.value) >> 23);
    return ms5611Temperature;
}

///////////////////////////////////////////////////////////////////////////////
// Read Temperature Request Pressure
///////////////////////////////////////////////////////////////////////////////

void readTemperature()
{
    //setSPIdivisor(2);  // 18 MHz SPI clock

    ENABLE_MS5611;
    spiTransferByte(0x00);
    d2.bytes[2] = spiTransferByte(0x00);
    d2.bytes[1] = spiTransferByte(0x00);
    d2.bytes[0] = spiTransferByte(0x00);
    DISABLE_MS5611;
    //delayMicroseconds(10);
    calculateTemperature();
}

void readPressure()
{
    //setSPIdivisor(2);  // 18 MHz SPI clock

    ENABLE_MS5611;
    spiTransferByte(0x00);
    d1.bytes[2] = spiTransferByte(0x00);
    d1.bytes[1] = spiTransferByte(0x00);
    d1.bytes[0] = spiTransferByte(0x00);
    DISABLE_MS5611;
    //delayMicroseconds(10);
}

void requestTemperature()
{
    //setSPIdivisor(2);  // 18 MHz SPI clock

    ENABLE_MS5611;                      // Request temperature conversion
#if   (OSR ==  256)
    spiTransferByte(0x50);
#elif (OSR ==  512)
    spiTransferByte(0x52);
#elif (OSR == 1024)
    spiTransferByte(0x54);
#elif (OSR == 2048)
    spiTransferByte(0x56);
#elif (OSR == 4096)
    spiTransferByte(0x58);
#endif
    DISABLE_MS5611;

    //delayMicroseconds(1);

}
void requestPressure()
{
    //setSPIdivisor(2);  // 18 MHz SPI clock

    ENABLE_MS5611;                      // Request pressure conversion
#if   (OSR ==  256)
    spiTransferByte(0x40);
#elif (OSR ==  512)
    spiTransferByte(0x42);
#elif (OSR == 1024)
    spiTransferByte(0x44);
#elif (OSR == 2048)
    spiTransferByte(0x46);
#elif (OSR == 4096)
    spiTransferByte(0x48);
#endif
    DISABLE_MS5611;

    //delayMicroseconds(1);
}


///////////////////////////////////////////////////////////////////////////////
// Calculate Pressure Altitude
///////////////////////////////////////////////////////////////////////////////

void calculatePressureAltitude(int32_t *pressure, int32_t *temperature)
{
    int64_t offset;
    int64_t offset2 = 0;
    int64_t sensitivity;
    int64_t sensitivity2 = 0;
    int64_t f;
    int32_t p;

    int32_t ms5611Temp2 = 0;

    offset = ((int64_t)c2.value << 16) + (((int64_t)c4.value * dT) >> 7);
    sensitivity = ((int64_t)c1.value << 15) + (((int64_t)c3.value * dT) >> 8);

    if (ms5611Temperature < 2000) {
        ms5611Temp2 = (dT * dT) >> 31;

        f = ms5611Temperature - 2000;
        f = f * f;
        offset2 = 5 * f >> 1;
        sensitivity2 = 5 * f >> 2;

        if (ms5611Temperature < -1500) {
            f = (ms5611Temperature + 1500);
            f = f * f;
            offset2 += 7 * f;
            sensitivity2 += 11 * f >> 1;
        }

        ms5611Temperature -= ms5611Temp2;

        offset -= offset2;
        sensitivity -= sensitivity2;
    }

    p = (((d1.value * sensitivity) >> 21) - offset) >> 15;
    if (pressure)
        *pressure = p;
    if (temperature)
        *temperature = ms5611Temperature;
//    return  (44330.0f * (1.0f - pow((float)p / 101325.0f, 1.0f / 5.255f)));
    //cliPrintF("%9.4f\n\r", sensors.pressureAlt50Hz);
}

///////////////////////////////////////////////////////////////////////////////
// Pressure Initialization
///////////////////////////////////////////////////////////////////////////////

bool ms5611DetectSpi(baro_t *baro)
{
    spiResetErrorCounter();
    setSPIdivisor(64);  // 18 MHz SPI clock

    ENABLE_MS5611;   // Reset Device
    spiTransferByte(0x1E);
    delay(3);
    DISABLE_MS5611;

    delay(150);

    ENABLE_MS5611;   // Read Calibration Data C1
    spiTransferByte(0xA2);
    c1.bytes[1] = spiTransferByte(0x00);
    c1.bytes[0] = spiTransferByte(0x00);
    DISABLE_MS5611;

    delayMicroseconds(10);

    ENABLE_MS5611;   // Read Calibration Data C2
    spiTransferByte(0xA4);
    c2.bytes[1] = spiTransferByte(0x00);
    c2.bytes[0] = spiTransferByte(0x00);
    DISABLE_MS5611;

    delayMicroseconds(10);

    ENABLE_MS5611;   // Read Calibration Data C3
    spiTransferByte(0xA6);
    c3.bytes[1] = spiTransferByte(0x00);
    c3.bytes[0] = spiTransferByte(0x00);
    DISABLE_MS5611;

    delayMicroseconds(10);

    ENABLE_MS5611;   // Read Calibration Data C4
    spiTransferByte(0xA8);
    c4.bytes[1] = spiTransferByte(0x00);
    c4.bytes[0] = spiTransferByte(0x00);
    DISABLE_MS5611;

    delayMicroseconds(10);

    ENABLE_MS5611;   // Read Calibration Data C5
    spiTransferByte(0xAA);
    c5.bytes[1] = spiTransferByte(0x00);
    c5.bytes[0] = spiTransferByte(0x00);
    DISABLE_MS5611;

    delayMicroseconds(10);

    ENABLE_MS5611;   // Read Calibration Data C6
    spiTransferByte(0xAC);
    c6.bytes[1] = spiTransferByte(0x00);
    c6.bytes[0] = spiTransferByte(0x00);
    DISABLE_MS5611;

    if (((int8_t)c6.bytes[1]) == -1 || spiGetErrorCounter() != 0) {
        spiResetErrorCounter();
        return false;
    }

    delay(10);

    requestTemperature();

    delay(10);

    baro->ut_delay = 10000;
    baro->up_delay = 10000;
    baro->start_ut = requestTemperature;
    baro->get_ut = readTemperature;
    baro->start_up = requestPressure;
    baro->get_up = readPressure;
    baro->calculate = calculatePressureAltitude;

    return true;
}
