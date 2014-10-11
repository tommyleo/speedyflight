#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>


void failureMode(uint8_t mode);

void delayMicroseconds(uint32_t us);

void delay(uint32_t ms);

uint32_t micros(void);

uint32_t millis(void);

void systemReset(bool toBootloader);

// make it safe for flash write operation
void systemPause(void);
void systemUnPause(void);

// current crystal frequency - 8 or 12MHz
extern uint32_t hse_value;

extern int hw_revision;
