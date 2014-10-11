#pragma once

///////////////////////////////////////////////////////////////////////////////
// EEPROM Defines
////////////////////////////////////////////////////////////////////////////////

#define WRITE_ENABLE                    0x06
#define WRITE_DISABLE                   0x04
#define READ_STATUS_REGISTER            0x05
#define WRITE_STATUS_REGISTER           0x01
#define READ_DATA                       0x03
#define FAST_READ                       0x0B
#define FAST_READ_DUAL_OUTPUT           0x3B
#define FAST_READ_DUAL_IO               0xBB
#define PAGE_PROGRAM_256_BYTES          0x02
#define SECTOR_ERASE_4K                 0x20
#define BLOCK_ERASE_32KB                0x52
#define BLOCK_ERASE_64KB                0xD8
#define CHIP_ERASE                      0xC7
#define POWER_DOWN                      0xB9
#define RELEASE_POWER_DOWN              0xAB
#define MANUFACTURER_DEVICE_ID          0x90
#define MANUFACTURER_DEVICE_ID_DUAL_IO  0x92
#define JEDEC_ID                        0x9F
#define READ_UNIQUE_ID                  0x4B

#define EEPROM_SPI          SPI1
#define EEPROM_CS_GPIO      GPIOE
#define EEPROM_CS_PIN       GPIO_Pin_12
#define EEPROM_CS_GPIO_CLK  RCC_AHBPeriph_GPIOE
#define ENABLE_EEPROM       GPIO_ResetBits(EEPROM_CS_GPIO, EEPROM_CS_PIN)
#define DISABLE_EEPROM      GPIO_SetBits(EEPROM_CS_GPIO,   EEPROM_CS_PIN)
