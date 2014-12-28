#include "board.h"

USB_OTG_CORE_HANDLE    USB_OTG_dev;
uint8_t usbDeviceConfigured = false;
#define USB_TIMEOUT  50

static uartPort_t uartPortUSB;

void usbSetBaudRate(serialPort_t *instance, uint32_t baudRate)
{
// TODO restart usb with baudrate
}

void usbSetMode(serialPort_t *instance, portMode_t mode)
{
    // TODO check if really nothing to do
}

bool isUsbTransmitBufferEmpty(serialPort_t *instance)
{
    return true;
}

static void usbPrintf(void *p, char c)
{
    usbPrint( NULL, c);
}

///////////////////////////////////////////////////////////////////////////////
// CLI Available
///////////////////////////////////////////////////////////////////////////////

uint32_t usbAvailable(serialPort_t *instance)
{
    return (VCP_DataRX_IsCharReady() != 0);
}

///////////////////////////////////////////////////////////////////////////////
// CLI Read
///////////////////////////////////////////////////////////////////////////////

uint8_t usbRead(serialPort_t *instance)
{
    uint8_t buf;

    if (VCP_get_char(&buf))
            return buf;
        else
            return(0);
}

///////////////////////////////////////////////////////////////////////////////
// CLI Print
///////////////////////////////////////////////////////////////////////////////

void usbPrintStr(const char *str)
{
	if (usbDeviceConfigured == true)
	{
		VCP_send_buffer((uint8_t*)str, strlen(str));
	}
}

void usbPrint(serialPort_t *instance, uint8_t c)
{
	if (usbDeviceConfigured == true)
	{
		VCP_send_buffer(&c, 1);
        USB_OTG_BSP_uDelay(250); //Just for Baseflight configurator, VRBrain is too fast!!!
	}
}

const struct serialPortVTable usbVTable[] = { { usbPrint, usbAvailable, usbRead, usbSetBaudRate, isUsbTransmitBufferEmpty, usbSetMode, } };

serialPort_t *usbInit(void)
{
    uartPort_t *s;

	USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb, &USR_cb);

    init_printf(NULL, usbPrintf);

    s = &uartPortUSB;
    s->port.vTable = usbVTable;
    usbDeviceConfigured = true;
    return (serialPort_t *)s;
}
