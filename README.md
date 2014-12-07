# speedyFlight 2.31  

# VRBrain - STM32F4XX - 168 Mhz

32 bit fork of the baseflight MultiWii RC flight controller firmware

## Warning, this is a beta version, do not fly and remove the propeller, it only proves the configuration of VRBrain with the GCS

# Sensors

MPU6000 (SPI Gyro + Acc)

HMC5883 (Compass)

MS5611 (SPI Barometer)

EEPROM (SPI For local parameters storage)

8 RC Input PWM (Test ok) or  Sat Spektrum (Test ok)

8 RC Output at 490 hz

3 Seriale ports

GPS Ublox OK


## Wires (you can switch from Arducopter without change the PIN connection)

![alt tag](https://raw.github.com/tommyleo/speedyflight/master/images/speedyFlight_collegamenti.png)


## Configuration Tool

To configure speedyFlight you should use:

Baseflight configurator
https://chrome.google.com/webstore/detail/baseflight-configurator/mppkgnedeapfejgfimkdoninnofofigk


## Features

For a list of features, changes and some discussion please review the thread on MultiWii forums and consult the documenation.

https://github.com/multiwii/baseflight/wiki/CLI-Variables


## Develop with Eclipse

You need an ARM toolchain. 

Add new target, with Make target: TARGET=VRBRAIN



### license

speedyFlight is licensed under *GPL V3* (just like MultiWii and BaseFlight code it originated from), with all the conditions GPL V3 implies.
https://github.com/multiwii/baseflight (GPLv3) 