#include "board.h"
#include "uart.h"

static uartPort_t uartPort1;
static uartPort_t uartPort2;
static uartPort_t uartPort3;
static uartPort_t uartPort6;

void uartPause(int n)
{
    switch (n) {
        case 3:
            USART_Cmd(USART3, DISABLE);

            break;
        default:
            break;
    }

}

void uartUnPause(int n)
{
    switch (n) {
        case 3:
            USART_Cmd(USART3, ENABLE);
            break;
        default:
            break;
    }
}


uartPort_t *serialUSART1(uint32_t baudRate, portMode_t mode)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    uartPort_t *s;
    static volatile uint8_t rx1Buffer[UART1_RX_BUFFER_SIZE];
    static volatile uint8_t tx1Buffer[UART1_TX_BUFFER_SIZE];

    s = &uartPort1;
    s->port.vTable = uartVTable;
    s->port.baudRate = baudRate;
    s->port.rxBufferSize = UART1_RX_BUFFER_SIZE;
    s->port.txBufferSize = UART1_TX_BUFFER_SIZE;
    s->port.rxBuffer = rx1Buffer;
    s->port.txBuffer = tx1Buffer;
    s->USARTx = USART1;

	/* Enable GPIO clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	/* Enable UART clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

    if (mode & MODE_TX) {
        GPIO_PinAFConfig(UART1_TX_GPIO, UART1_TX_PINSOURCE, GPIO_AF_USART1);
        GPIO_InitStructure.GPIO_Pin = UART1_TX_PIN;
        GPIO_Init(UART1_TX_GPIO, &GPIO_InitStructure);
    }

    if (mode & MODE_RX) {
        GPIO_PinAFConfig(UART1_RX_GPIO, UART1_RX_PINSOURCE, GPIO_AF_USART1);
        GPIO_InitStructure.GPIO_Pin = UART1_RX_PIN;
        GPIO_Init(UART1_RX_GPIO, &GPIO_InitStructure);
    }

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    return s;
}

uartPort_t *serialUSART2(uint32_t baudRate, portMode_t mode)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

	uartPort_t *s;

	static volatile uint8_t rx2Buffer[UART2_RX_BUFFER_SIZE];
    static volatile uint8_t tx2Buffer[UART2_TX_BUFFER_SIZE];

    s = &uartPort2;
    s->port.vTable = uartVTable;
    s->port.baudRate = baudRate;
    s->port.rxBufferSize = UART2_RX_BUFFER_SIZE;
    s->port.txBufferSize = UART2_TX_BUFFER_SIZE;
    s->port.rxBuffer = rx2Buffer;
    s->port.txBuffer = tx2Buffer;
    s->USARTx = USART2;

	/* Enable GPIO clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

    /* Enable UART clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

    if (mode & MODE_TX) {
        GPIO_InitStructure.GPIO_Pin = UART2_TX_PIN;
        GPIO_PinAFConfig(UART2_TX_GPIO, UART2_TX_PINSOURCE, GPIO_AF_USART2);
        GPIO_Init(UART2_TX_GPIO, &GPIO_InitStructure);
    }

    if (mode & MODE_RX) {
        GPIO_InitStructure.GPIO_Pin = UART2_RX_PIN;
        GPIO_PinAFConfig(UART2_RX_GPIO, UART2_RX_PINSOURCE, GPIO_AF_USART2);
        GPIO_Init(UART2_RX_GPIO, &GPIO_InitStructure);
    }

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    return s;
}

uartPort_t *serialUSART3(uint32_t baudRate, portMode_t mode)
{
	uartPort_t *s;
    static volatile uint8_t rx3Buffer[UART3_RX_BUFFER_SIZE];
    static volatile uint8_t tx3Buffer[UART3_TX_BUFFER_SIZE];
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    s = &uartPort3;
    s->port.vTable = uartVTable;
    s->port.baudRate = baudRate;
    s->port.rxBufferSize = UART3_RX_BUFFER_SIZE;
    s->port.txBufferSize = UART3_TX_BUFFER_SIZE;
    s->port.rxBuffer = rx3Buffer;
    s->port.txBuffer = tx3Buffer;
    s->USARTx = USART3;

	/* Enable GPIO clock */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	/* Enable UART clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

    if (mode & MODE_TX) {
         GPIO_InitStructure.GPIO_Pin = UART3_TX_PIN;
         GPIO_PinAFConfig(UART3_TX_GPIO, UART3_TX_PINSOURCE, GPIO_AF_USART3);
         GPIO_Init(UART3_TX_GPIO, &GPIO_InitStructure);
     }

     if (mode & MODE_RX) {
         GPIO_InitStructure.GPIO_Pin = UART3_RX_PIN;
         GPIO_PinAFConfig(UART3_RX_GPIO, UART3_RX_PINSOURCE, GPIO_AF_USART3);
         GPIO_Init(UART3_RX_GPIO, &GPIO_InitStructure);
     }

     NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
     NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
     NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
     NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
     NVIC_Init(&NVIC_InitStructure);

     return s;
}

uartPort_t *serialUSART6(uint32_t baudRate, portMode_t mode)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

	uartPort_t *s;

	static volatile uint8_t rx6Buffer[UART6_RX_BUFFER_SIZE];
    static volatile uint8_t tx6Buffer[UART6_TX_BUFFER_SIZE];

    s = &uartPort6;
    s->port.vTable = uartVTable;
    s->port.baudRate = baudRate;
    s->port.rxBufferSize = UART6_RX_BUFFER_SIZE;
    s->port.txBufferSize = UART6_TX_BUFFER_SIZE;
    s->port.rxBuffer = rx6Buffer;
    s->port.txBuffer = tx6Buffer;
    s->USARTx = USART6;

	//Enable GPIO clock
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	//Enable UART clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

    if (mode & MODE_TX) {
        GPIO_InitStructure.GPIO_Pin = UART6_TX_PIN;
        GPIO_PinAFConfig(UART6_TX_GPIO, UART6_TX_PINSOURCE, GPIO_AF_USART6);
        GPIO_Init(UART6_TX_GPIO, &GPIO_InitStructure);
    }

    if (mode & MODE_RX) {
        GPIO_InitStructure.GPIO_Pin = UART6_RX_PIN;
        GPIO_PinAFConfig(UART6_RX_GPIO, UART6_RX_PINSOURCE, GPIO_AF_USART6);
        GPIO_Init(UART6_RX_GPIO, &GPIO_InitStructure);
    }

    NVIC_InitStructure.NVIC_IRQChannel = USART6_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    return s;
}

serialPort_t *uartOpen(USART_TypeDef *USARTx, serialReceiveCallbackPtr callback, uint32_t baudRate, portMode_t mode, serialInversion_e inversion)
{
    USART_InitTypeDef USART_InitStructure;

    uartPort_t *s = NULL;

    if (USARTx == USART1) {
        s = serialUSART1(baudRate, mode);
    } else if (USARTx == USART2) {
        s = serialUSART2(baudRate, mode);
    } else if (USARTx == USART3) {
        s = serialUSART3(baudRate, mode);
    } else if (USARTx == USART6) {
        s = serialUSART6(baudRate, mode);
    } else {
        return (serialPort_t *)s;
    }

    s->txDMAEmpty = true;
    s->port.rxBufferHead = s->port.rxBufferTail = 0;
    s->port.txBufferHead = s->port.txBufferTail = 0;
	s->port.callback = callback;
    s->port.mode = mode;
    s->port.baudRate = baudRate;
    s->port.inversion = inversion/*SERIAL_NOT_INVERTED*/; // Not used on STM32F4

    USART_InitStructure.USART_BaudRate = baudRate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    if (mode & MODE_SBUS) {
        USART_InitStructure.USART_StopBits = USART_StopBits_2;
        USART_InitStructure.USART_Parity = USART_Parity_Even;
    } else {
        USART_InitStructure.USART_StopBits = USART_StopBits_1;
        USART_InitStructure.USART_Parity = USART_Parity_No;
    }
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = 0;
    if (mode & MODE_RX)
        USART_InitStructure.USART_Mode |= USART_Mode_Rx;
    if (mode & MODE_TX)
        USART_InitStructure.USART_Mode |= USART_Mode_Tx;

    USART_Init(USARTx, &USART_InitStructure);

    USART_Cmd(USARTx, ENABLE);

    // Receive IRQ
    if (mode & MODE_RX) {
		USART_ClearITPendingBit(s->USARTx, USART_IT_RXNE);
		USART_ITConfig(USARTx, USART_IT_RXNE, ENABLE);
    }

    // Transmit IRQ
    if (mode & MODE_TX) {
		USART_ITConfig(USARTx, USART_IT_TXE, ENABLE);
    }

    return (serialPort_t *)s;
}

void uartSetBaudRate(serialPort_t *instance, uint32_t baudRate)
{
    uartPort_t *uartPort = (uartPort_t *)instance;
    uartPort->port.baudRate = baudRate;
    uartOpen(uartPort->USARTx, uartPort->port.callback, uartPort->port.baudRate, uartPort->port.mode, uartPort->port.inversion);
}

void uartSetMode(serialPort_t *instance, portMode_t mode)
{
    uartPort_t *uartPort = (uartPort_t *)instance;
    uartPort->port.mode = mode;
    uartOpen(uartPort->USARTx, uartPort->port.callback, uartPort->port.baudRate, uartPort->port.mode, uartPort->port.inversion);
}


uint32_t uartTotalBytesWaiting(serialPort_t *instance)
{
    uartPort_t *s = (uartPort_t*)instance;
	return (uint32_t)s->port.rxBufferTail != s->port.rxBufferHead;
}

bool isUartTransmitBufferEmpty(serialPort_t *instance)
{
	uartPort_t *s = (uartPort_t*)instance;
	return s->port.txBufferTail == s->port.txBufferHead;
}

uint8_t uartRead(serialPort_t *instance)
{
    uint8_t ch;
    uartPort_t *s = (uartPort_t *)instance;

    ch = s->port.rxBuffer[s->port.rxBufferTail];
	s->port.rxBufferTail = (s->port.rxBufferTail + 1) % s->port.rxBufferSize;

    return ch;
}

void uartWrite(serialPort_t *instance, uint8_t ch)
{
    uartPort_t *s = (uartPort_t *)instance;

    USART_SendData(s->USARTx, ch);
    while(USART_GetFlagStatus(s->USARTx, USART_FLAG_TC) == RESET){}

    /*
    s->port.txBuffer[s->port.txBufferHead] = ch;
    s->port.txBufferHead = (s->port.txBufferHead + 1) % s->port.txBufferSize;
	USART_ITConfig(s->USARTx, USART_IT_TXE, ENABLE);
	*/
	//USB_OTG_BSP_uDelay(250); //Just for Baseflight configurator, VRBrain is too fast!!!
}

const struct serialPortVTable uartVTable[] = {
	{
		uartWrite,
		uartTotalBytesWaiting,
		uartRead,
		uartSetBaudRate,
		isUartTransmitBufferEmpty,
		uartSetMode,
	}
};

void usartIrqHandler(uartPort_t *s)
{
    uint32_t ISR = s->USARTx->SR;

    if (ISR & USART_FLAG_RXNE) {
        if (s->port.callback) {
            s->port.callback(s->USARTx->DR);
        } else {
            s->port.rxBuffer[s->port.rxBufferHead] = s->USARTx->DR;
            s->port.rxBufferHead = (s->port.rxBufferHead + 1) % s->port.rxBufferSize;
        }
    }

    if (ISR & USART_FLAG_TXE) {
        if (s->port.txBufferTail != s->port.txBufferHead) {
            USART_SendData(s->USARTx, s->port.txBuffer[s->port.txBufferTail]);
            while(USART_GetFlagStatus(s->USARTx, USART_FLAG_TC) == RESET){}
            s->port.txBufferTail = (s->port.txBufferTail + 1) % s->port.txBufferSize;
        } else {
            USART_ITConfig(s->USARTx, USART_IT_TXE, DISABLE);
        }
    }

    if (ISR & USART_FLAG_ORE) {
        USART_ClearITPendingBit(s->USARTx, USART_IT_ORE);
    }
}

void USART1_IRQHandler(void)
{
    uartPort_t *s = &uartPort1;
    usartIrqHandler(s);
}

void USART2_IRQHandler(void)
{
    uartPort_t *s = &uartPort2;
    usartIrqHandler(s);
}

void USART3_IRQHandler(void)
{
    uartPort_t *s = &uartPort3;
    usartIrqHandler(s);
}

void USART6_IRQHandler(void)
{
    uartPort_t *s = &uartPort6;
    usartIrqHandler(s);
}

