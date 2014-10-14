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


## Configuration Tool

To configure speedyFlight you should use the BaseFlight configurator GUI tool (Windows/OSX/Linux) that can be found here:

https://chrome.google.com/webstore/detail/baseflight-configurator/mppkgnedeapfejgfimkdoninnofofigk


## Features

For a list of features, changes and some discussion please review the thread on MultiWii forums and consult the documenation.

http://www.multiwii.com/forum/viewtopic.php?f=23&t=5149


## Develop with Eclipse

You need an ARM toolchain. 

Add new target, with Make target:  TARGET=VRBRAIN


## Wires

![alt tag](https://raw.github.com/tommyleo/speedyflight/master/images/speedyFlight_collegamenti.png)



### license

speedyFlight is licensed under *GPL V3* (just like MultiWii code it originated from), with all the conditions GPL V3 implies,


with the following exception:

1) You can:

1.1) Use the source to create improvements and bug-fixes to send to the author to be incorporated into baseflight.

1.2) Use it for review/educational purposes.

2) You can NOT:

2.1) Use the source to create derivative works. (That is, you can't release your own version of speedyFlight with your changes in it)

2.2) Compile your own version and sell it.

2.3) Distribute unmodified, modified source or compiled versions of speedyFlight without first obtaining permission from the author.

2.4) Use any of the code or other part of speedyFlight in anything other than speedyFlight.

3) All code submitted to the project:

3.1) Will be automatically GPL V3 licensed whether contributor's names are  "dominic clifton", "Tommaso Leognani" or not.

3.2) Will become the property of speedyFlight author.


note that above exception is strictly name-based and does not apply to general developers who wish to contribute to speedyFlight. 
