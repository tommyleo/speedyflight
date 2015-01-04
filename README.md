# speedyFlight 2.31  
32 bit fork of the baseflight MultiWii RC flight controller firmware

# VR μBrain - STM32F4XX - 168 Mhz 
## Looptime = 50 (Main Cycle Time 50μs) (20 KHz by default)

Web reference: http://vrbrain.wordpress.com/vr-microbrain/

## Warning, this is a beta version, do not fly and remove the propeller, it only proves the configuration of VR μBrain with the GCS

# Videos

## <a href="https://www.youtube.com/watch?v=Y6o8bMnIQCA&feature=youtu.be" target="blank">Test GCS</a>

## <a href="https://www.youtube.com/watch?v=ftrmEvXqNM0" target="blank">Test flight</a>

# Sensors

MPU6000 (SPI Gyro + Acc) (Test OK)

HMC5883 (I2C External Compass) (Test OK)

MS5611 (SPI Barometer) (Test OK)

8 RC Input PWM, Sat Spektrum, SBUS (Test OK)

8 RC Output up to 2 Khz (Test OK)

3 Seriale ports (Test OK)

GPS Ublox (Test OK)


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

To configure speedyFlight you should use <a href="https://chrome.google.com/webstore/detail/baseflight-configurator/mppkgnedeapfejgfimkdoninnofofigk" target="blank">Baseflight - Configurator</a>


## Features

For a list of features, changes and some discussion please review the thread on MultiWii forums and consult the documenation.

https://github.com/multiwii/baseflight/wiki/CLI-Variables


## Develop with Eclipse

You need an ARM toolchain. 

Add new target, with Make target: TARGET=VRBRAIN



### license

speedyFlight is licensed under *GPL V3* (just like MultiWii and BaseFlight code it originated from), with all the conditions GPL V3 implies.
https://github.com/multiwii/baseflight (GPLv3) 