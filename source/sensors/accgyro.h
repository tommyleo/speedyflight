#pragma once

extern uint16_t acc_1G;

typedef void (*sensorInitFuncPtr2)(void);                    // sensor init prototype
typedef void (*sensorReadFuncPtr2)(int16_t *data);           // sensor read and align prototype

typedef struct gyro_s {
    sensorInitFuncPtr2 init;                                 // initialize function
    sensorReadFuncPtr2 read;                                 // read 3 axis data function
    sensorReadFuncPtr2 temperature;                          // read temperature if available
    float scale;                                             // scalefactor
} gyro_t;

typedef struct acc_s {
    sensorInitFuncPtr2 init;                                 // initialize function
    sensorReadFuncPtr2 read;                                 // read 3 axis data function
    char revisionCode;                                       // a revision code for the sensor, if known
} acc_t;
