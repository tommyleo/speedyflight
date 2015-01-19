###############################################################################
# "THE BEER-WARE LICENSE" (Revision 42):
# <msmith@FreeBSD.ORG> wrote this file. As long as you retain this notice you
# can do whatever you want with this stuff. If we meet some day, and you think
# this stuff is worth it, you can buy me a beer in return
###############################################################################
#
# Makefile for building the firmware.
#
# Invoke this with 'make help' to see the list of supported targets.
# 

###############################################################################
# Things that the user might override on the commandline
#


# The target to build
TARGET		?=  VRBRAIN

# The name of the build
BRANCH_NAME     = dev

# Compile-time options
OPTIONS		?=

# Debugger options, must be empty or GDB
DEBUG ?=

# Serial port/Device for flashing
SERIAL_DEVICE	?= /dev/ttyUSB0


###############################################################################
# Things that need to be maintained as the source changes
#

VALID_TARGETS	 = VRBRAIN


# Common working directories
#$(ROOT)		 = C:/code/speedyFlight

ROOT         = $(dir $(lastword $(MAKEFILE_LIST)))
SRC_DIR		 = $(ROOT)/source
OBJECT_DIR	 = $(ROOT)/obj
BIN_DIR		 = $(ROOT)/obj
CMSIS_DIR	 = $(ROOT)/lib/cmsis
CMSISB_DIR	 = $(ROOT)/lib/cmsis_boot
FATFS_DIR	 = $(ROOT)/lib/fatfs
INCLUDE_DIRS = $(SRC_DIR)

# Common Search path for sources
VPATH		 = $(SRC_DIR)

DEVICE_MCUNAME = stm32f4xx
DIR_STDPERIPH  = $(ROOT)/lib/cmsis_lib
DIR_USBFS      = $(ROOT)/lib/usb_lib/core
DIR_USBCDC     = $(ROOT)/lib/usb_lib/cdc
DIR_USBOTG     = $(ROOT)/lib/usb_lib/otg

CMSIS_SRC	  = $(notdir $(wildcard $(CMSISB_DIR)/*.c))

VPATH		:= $(VPATH):$(CMSIS_DIR):$(CMSISB_DIR):$(FATFS_DIR)

INCLUDE_DIRS := $(INCLUDE_DIRS) \
		   $(DIR_STDPERIPH)/include \
		   $(DIR_USBFS) \
		   $(DIR_USBCDC) \
		   $(DIR_USBOTG) \
		   $(CMSIS_DIR) \
		   $(CMSISB_DIR) \
		   $(FATFS_DIR) \
		   $(SRC_DIR)/drivers/vcp_$(DEVICE_MCUNAME)


LD_SCRIPT	 = $(ROOT)/stm32_flash_f4xx.ld

ARCH_FLAGS	 = -mthumb -mthumb-interwork -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -fsingle-precision-constant -Wdouble-promotion

#DEVICE_FLAGS = -DSTM32F4XX -DARM_MATH_CM4  -DUSE_USB_OTG_FS
DEVICE_FLAGS = -DSTM32F407VG -DSTM32F4XX -DSTM32F40_41xxx -DUSE_USB_OTG_FS -DARM_MATH_CM4 -DHSE_VALUE=8000000


# Search path and source files for the ST stdperiph library
VPATH		:= $(VPATH):$(DIR_STDPERIPH)/source:$(DIR_USBFS):$(DIR_USBOTG):$(DIR_USBCDC)

STDPERIPH_SRC = $(notdir $(wildcard $(DIR_STDPERIPH)/source/*.c))
USBPERIPH_SRC = $(notdir $(wildcard $(DIR_USBFS)/*.c))
USBOTGPERIPH_SRC = $(notdir $(wildcard $(DIR_USBOTG)/*.c))
USBCDCPERIPH_SRC = $(notdir $(wildcard $(DIR_USBCDC)/*.c))
FATFS_SRC = $(notdir $(wildcard $(FATFS_DIR)/*.c))

EXCLUDES = stm32f4xx_crc.c\
stm32f4xx_can.c 

STDPERIPH_SRC := $(filter-out ${EXCLUDES}, $(STDPERIPH_SRC))

EXCLUDES = usb_otg.c\
usb_bsp_template.c \
usb_hcd.c \
usb_hcd_int.c \

USBOTGPERIPH_SRC := $(filter-out ${EXCLUDES}, $(USBOTGPERIPH_SRC))

DEVICE_STDPERIPH_SRC = $(USBPERIPH_SRC) \
$(STDPERIPH_SRC) \
$(USBOTGPERIPH_SRC) \
$(USBCDCPERIPH_SRC)


MW_SRC	 = buzzer.c \
		cli.c \
		config.c \
		gps.c \
		imu.c \
		main.c \
		mw.c \
		mixer.c \
		printf.c \
		rxmsp.c \
		rxsbus.c \
		rxspektrum.c \
		rxsumd.c \
		sensors.c \
		serial.c \
		utils.c \
		telemetry_common.c \
		telemetry_frsky.c \
		telemetry_hott.c \
		blackbox.c \
		$(FATFS_SRC) \
		$(CMSIS_SRC) \
		$(DEVICE_STDPERIPH_SRC) \
		lib/cmsis_boot/startup/startup_$(DEVICE_MCUNAME).c
		
VRBRAIN_SRC = $(MW_SRC) \
	    drivers/adc_stm32f4xx.c \
		drivers/spi1.c \
		drivers/spi2.c \
		drivers/crc.c \
		drivers/pwm.c \
		drivers/serial.c \
		drivers/softserial.c \
		drivers/timer.c \
		sensors/i2c_hmc5883l.c \
 		sensors/spi_mpu6000.c \
		sensors/spi_ms5611.c\
		drivers/adc_$(DEVICE_MCUNAME).c \
		drivers/gpio_$(DEVICE_MCUNAME).c \
		drivers/i2c_$(DEVICE_MCUNAME).c \
		drivers/uart_$(DEVICE_MCUNAME).c \
		drivers/usb_$(DEVICE_MCUNAME).c \
		drivers/vcp_$(DEVICE_MCUNAME)/stm32f4xx_it.c \
		drivers/vcp_$(DEVICE_MCUNAME)/usb_bsp.c \
		drivers/vcp_$(DEVICE_MCUNAME)/usbd_cdc_vcp.c \
		drivers/vcp_$(DEVICE_MCUNAME)/usbd_desc.c \
		drivers/vcp_$(DEVICE_MCUNAME)/usbd_usr.c


		
# In some cases, %.s regarded as intermediate file, which is actually not.
# This will prevent accidental deletion of startup code.
.PRECIOUS: %.s


###############################################################################
# Things that might need changing to use different tools
#

# Tool names
CC		= arm-none-eabi-gcc
OBJCOPY	= arm-none-eabi-objcopy

ifeq ($(DEBUG),GDB)
OPTIMIZE = -O0
LTO_FLAGS = $(OPTIMIZE)
else
OPTIMIZE = -Os
LTO_FLAGS = -flto -fuse-linker-plugin $(OPTIMIZE)
endif

DEBUG_FLAGS = -ggdb3

#
# Tool options.
BASE_CFLAGS	 = $(ARCH_FLAGS) \
		   $(LTO_FLAGS) \
		   $(addprefix -D,$(OPTIONS)) \
		   $(addprefix -I,$(INCLUDE_DIRS)) \
		   $(addprefix -isystem,$(CMSIS_DIR)/Include) \
		   $(addprefix -isystem,$(DIR_USBFS)/inc) \
		   $(addprefix -isystem,$(DIR_USBOTG)/inc) \
		   $(DEBUG_FLAGS) \
		   -std=gnu99 \
		   -Wall -Wextra -Wshadow -Wunsafe-loop-optimizations -Wno-ignored-qualifiers \
		   -ffunction-sections \
		   -fdata-sections \
		   $(DEVICE_FLAGS) \
		   -DUSE_STDPERIPH_DRIVER \
		   -D$(TARGET) \
		   -D'__BRANCH_NAME__="$(BRANCH_NAME)"' 

ASFLAGS		 = $(ARCH_FLAGS) \
		   -x assembler-with-cpp \
		   $(addprefix -I,$(INCLUDE_DIRS))

# XXX Map/crossref output?
LDFLAGS		 = -lm \
		   -nostartfiles \
		   --specs=nano.specs \
		   -lc -lnosys \
		   $(ARCH_FLAGS) \
		   -static \
		   -Wl,-gc-sections,-Map,$(TARGET_MAP) \
		   -T$(LD_SCRIPT)



###############################################################################
# No user-serviceable parts below
###############################################################################
#
# Things we will build
#

ifeq ($(filter $(TARGET),$(VALID_TARGETS)),)
$(error Target '$(TARGET)' is not valid, must be one of $(VALID_TARGETS))
endif


TARGET_HEX	 = $(BIN_DIR)/$(BRANCH_NAME)_$(TARGET).hex
TARGET_ELF	 = $(BIN_DIR)/$(BRANCH_NAME)_$(TARGET).elf
TARGET_OBJS	 = $(addsuffix .o,$(addprefix $(OBJECT_DIR)/$(TARGET)/,$(basename $($(TARGET)_SRC))))
TARGET_MAP   = $(OBJECT_DIR)/$(BRANCH_NAME)_$(TARGET).map

# List of buildable ELF files and their object dependencies.
# It would be nice to compute these lists, but that seems to be just beyond make.

$(TARGET_HEX): $(TARGET_ELF)
	$(OBJCOPY) -O ihex --set-start 0x8000000 $< $@

$(TARGET_ELF):  $(TARGET_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)


# Compile
$(OBJECT_DIR)/$(TARGET)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo %% $(notdir $<)
	@$(CC) -c -o $@ $(BASE_CFLAGS) $<

# Assemble
$(OBJECT_DIR)/$(TARGET)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo %% $(notdir $<)
	@$(CC) -c -o $@ $(ASFLAGS) $< 
$(OBJECT_DIR)/$(TARGET)/%.o): %.S
	@mkdir -p $(dir $@)
	@echo %% $(notdir $<)
	@$(CC) -c -o $@ $(ASFLAGS) $< 

clean:
	rm -f $(TARGET_HEX) $(TARGET_ELF) $(TARGET_OBJS) $(TARGET_MAP)
	rm -rf $(OBJECT_DIR)/$(TARGET)

flash_$(TARGET): $(TARGET_HEX)
	stty -F $(SERIAL_DEVICE) raw speed 115200 -crtscts cs8 -parenb -cstopb -ixon
	echo -n 'R' >$(SERIAL_DEVICE)
	stm32flash -w $(TARGET_HEX) -v -g 0x0 -b 115200 $(SERIAL_DEVICE)

flash: flash_$(TARGET)


unbrick_$(TARGET): $(TARGET_HEX)
	stty -F $(SERIAL_DEVICE) raw speed 115200 -crtscts cs8 -parenb -cstopb -ixon
	stm32flash -w $(TARGET_HEX) -v -g 0x0 -b 115200 $(SERIAL_DEVICE)

unbrick: unbrick_$(TARGET)

help:
	@echo ""
	@echo "Makefile for the $(BRANCH_NAME)_$(TARGET) firmware"
	@echo ""
	@echo "Usage:"
	@echo "        make [TARGET=<target>] [OPTIONS=\"<options>\"]"
	@echo ""
	@echo "Valid TARGET values are: $(VALID_TARGETS)"
	@echo ""
