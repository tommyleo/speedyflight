#ifndef LIBSTM32F_CRC_H
#define LIBSTM32F_CRC_H
#pragma once

#include "stm32f4xx_rcc.h"

typedef volatile uint32_t vuint32_t;

/*
  \brief    Reset the STM32s CRC Engine.
  \remark   4 NOPs are added to garrentee the reset is complete BEFORE data
            is pushed, because the reset does not seem to be treated as
            a dataregister write with the correct write stalling. In inlined
            code without the NOPs the first word was corrupted.
 */
static inline void crc32Reset(void)
  {


RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);

  CRC->CR = CRC_CR_RESET;
  __NOP(); // 4 Clocks to finish reset.
  __NOP();
  __NOP();
  __NOP();
  }

/*
  \brief    Write a 32bit word to the STMs CRC engine.
  \param x  The 32bit word to write.
 */
static inline void crc32Write(vuint32_t x)
  {
  CRC->DR = __RBIT(x);
  }

/*
  \brief   Read the STM32s CRC engine data register.
  \return  A 32 bit word read form the data register.
 */
static inline vuint32_t crc32Read (void)
  {
  return __RBIT(CRC->DR);
  }

void crc32Feed(uint32_t* start, uint32_t* end);
uint32_t crc32B(uint32_t* start, uint32_t* end);

enum { crcCheckVal = 0x2144DF1C };

// LIBSTM32F_CRC_H
#endif
