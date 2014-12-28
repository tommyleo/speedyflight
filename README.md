# speedyFlight 2.31  
32 bit fork of the baseflight MultiWii RC flight controller firmware

# VR μBrain - STM32F4XX - 168 Mhz 
## Looptime = 500 (2 KHz by default)

Web reference: http://vrbrain.wordpress.com/vr-microbrain/

## Warning, this is a beta version, do not fly and remove the propeller, it only proves the configuration of VR μBrain with the GCS

# Sensors

MPU6000 (SPI Gyro + Acc)

HMC5883 (Extern Compass)

MS5611 (SPI Barometer)

EEPROM (SPI For local parameters storage)

8 RC Input PWM (Test ok) or  Sat Spektrum (Test ok)

8 RC Output at 490 hz

3 Seriale ports

GPS Ublox OK


## Install firmware

- Disconnect the “Boot” Jumper located near the 10 pin connector.

- Open DfuSe Demonstration -  Download from here: https://vrbrain.googlecode.com/files/um0412.zip

![alt tag](https://raw.github.com/tommyleo/speedyflight/master/images/dfuse.png)

- Click on “Choose…” and select the .dfu file (obj/dev_VRBRAIN.dfu)

- Select “Upgrade” and click Yes. Now wait until the upload hase finished and Quit.



## Wires (you can switch from Arducopter without change the PIN connection)

![alt tag](https://raw.github.com/tommyleo/speedyflight/master/images/vrmicrobrain_top.png)

![alt tag](https://raw.github.com/tommyleo/speedyflight/master/images/vrmicrobrain_bottom1.png)


## Configuration Tool

To configure speedyFlight you should use:

Fork of Baseflight configurator - URL: https://github.com/tommyleo/baseflight-configurator

- Clone the repo to any local directory or download it as zip
- Start chromium or google chrome and go to tools -> extension
- Check the "Developer mode" checkbox
- Click on load unpacked extension and point it to the baseflight configurator directory (for example D:/baseflight-configurator)


## Features

For a list of features, changes and some discussion please review the thread on MultiWii forums and consult the documenation.

https://github.com/multiwii/baseflight/wiki/CLI-Variables


## Develop with Eclipse

You need an ARM toolchain. 

Add new target, with Make target: TARGET=VRBRAIN



### license

speedyFlight is licensed under *GPL V3* (just like MultiWii and BaseFlight code it originated from), with all the conditions GPL V3 implies.
https://github.com/multiwii/baseflight (GPLv3) 