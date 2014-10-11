#pragma once

serialPort_t *usbInit(void);

uint32_t usbAvailable(serialPort_t *instance);

uint8_t usbRead(serialPort_t *instance);

void usbPrint(serialPort_t *instance, uint8_t ch);
void usbPrintStr(const char *str);

//extern uint8_t usbDeviceConfigured;
