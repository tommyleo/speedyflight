#include "board.h"

// I2C2
// SCL  PB10
// SDA  PB11

static void i2c_er_handler(void);
static void i2c_ev_handler(void);

#define I2C_DEFAULT_TIMEOUT 30000
static volatile uint16_t i2cErrorCount = 0;

static volatile bool error = false;
static volatile bool busy;

static volatile uint8_t addr;
static volatile uint8_t reg;
static volatile uint8_t bytes;
static volatile uint8_t writing;
static volatile uint8_t reading;
static volatile uint8_t* write_p;
static volatile uint8_t* read_p;

static bool i2cHandleHardwareFailure(void)
{
    i2cErrorCount++;
    // reinit peripheral + clock out garbage
    i2cInit();
    return false;
}

bool i2cWriteBuffer(uint8_t addr_, uint8_t reg_, uint8_t len_, uint8_t *data)
{
    uint32_t timeout = I2C_DEFAULT_TIMEOUT;

    addr = addr_ << 1;
    reg = reg_;
    writing = 1;
    reading = 0;
    write_p = data;
    read_p = data;
    bytes = len_;
    busy = 1;
    error = false;
    
    if (!I2C2)
        return false;

    if (!(I2C2->CR2 & I2C_IT_EVT)) {                                    // if we are restarting the driver
        if (!(I2C2->CR1 & 0x0100)) {                                    // ensure sending a start
            while (I2C2->CR1 & 0x0200 && --timeout > 0) { ; }           // wait for any stop to finish sending
            if (timeout == 0)
                return i2cHandleHardwareFailure();
            I2C_GenerateSTART(I2C2, ENABLE);                            // send the start for the new job
        }
        I2C_ITConfig(I2C2, I2C_IT_EVT | I2C_IT_ERR, ENABLE);            // allow the interrupts to fire off again
    }

    timeout = I2C_DEFAULT_TIMEOUT;
    while (busy && --timeout > 0) { ; }
    if (timeout == 0)
        return i2cHandleHardwareFailure();

    return !error;
}

bool i2cWrite(uint8_t addr_, uint8_t reg_, uint8_t data)
{
    return i2cWriteBuffer(addr_, reg_, 1, &data);
}

bool i2cRead(uint8_t addr_, uint8_t reg_, uint8_t len, uint8_t* buf)
{
    uint32_t timeout = I2C_DEFAULT_TIMEOUT;

    addr = addr_ << 1;
    reg = reg_;
    writing = 0;
    reading = 1;
    read_p = buf;
    write_p = buf;
    bytes = len;
    busy = 1;
    error = false;

    if (!I2C2)
        return false;

    if (!(I2C2->CR2 & I2C_IT_EVT)) {                                    // if we are restarting the driver
        if (!(I2C2->CR1 & 0x0100)) {                                    // ensure sending a start
            while (I2C2->CR1 & 0x0200 && --timeout > 0) { ; }           // wait for any stop to finish sending
            if (timeout == 0)
                return i2cHandleHardwareFailure();
            I2C_GenerateSTART(I2C2, ENABLE);                            // send the start for the new job
        }
        I2C_ITConfig(I2C2, I2C_IT_EVT | I2C_IT_ERR, ENABLE);            // allow the interrupts to fire off again
    }

    timeout = I2C_DEFAULT_TIMEOUT;
    while (busy && --timeout > 0) { ; }
    if (timeout == 0)
        return i2cHandleHardwareFailure();

    return !error;
}

static void i2c_er_handler(void)
{
    // Read the I2C1 status register
    volatile uint32_t SR1Register = I2C2->SR1;

    if (SR1Register & 0x0F00)                                           // an error
        error = true;

    // If AF, BERR or ARLO, abandon the current job and commence new if there are jobs
    if (SR1Register & 0x0700) {
        (void)I2C2->SR2;                                                // read second status register to clear ADDR if it is set (note that BTF will not be set after a NACK)
        I2C_ITConfig(I2C2, I2C_IT_BUF, DISABLE);                        // disable the RXNE/TXE interrupt - prevent the ISR tailchaining onto the ER (hopefully)
        if (!(SR1Register & 0x0200) && !(I2C2->CR1 & 0x0200)) {         // if we dont have an ARLO error, ensure sending of a stop
            if (I2C2->CR1 & 0x0100) {                                   // We are currently trying to send a start, this is very bad as start, stop will hang the peripheral
                while (I2C2->CR1 & 0x0100) { ; }                        // wait for any start to finish sending
                I2C_GenerateSTOP(I2C2, ENABLE);                         // send stop to finalise bus transaction
                while (I2C2->CR1 & 0x0200) { ; }                        // wait for stop to finish sending
                i2cInit(I2C2);                                    // reset and configure the hardware
            } else {
                I2C_GenerateSTOP(I2C2, ENABLE);                         // stop to free up the bus
                I2C_ITConfig(I2C2, I2C_IT_EVT | I2C_IT_ERR, DISABLE);   // Disable EVT and ERR interrupts while bus inactive
            }
        }
    }
    I2C2->SR1 &= ~0x0F00;                                               // reset all the error bits to clear the interrupt
    busy = 0;
}

void i2c_ev_handler(void)
{
    static uint8_t subaddress_sent, final_stop;                         // flag to indicate if subaddess sent, flag to indicate final bus condition
    static int8_t index;                                                // index is signed -1 == send the subaddress
    uint8_t SReg_1 = I2C2->SR1;                                         // read the status register here

    if (SReg_1 & 0x0001) {                                              // we just sent a start - EV5 in ref manual
        I2C2->CR1 &= ~0x0800;                                           // reset the POS bit so ACK/NACK applied to the current byte
        I2C_AcknowledgeConfig(I2C2, ENABLE);                            // make sure ACK is on
        index = 0;                                                      // reset the index
        if (reading && (subaddress_sent || 0xFF == reg)) {              // we have sent the subaddr
            subaddress_sent = 1;                                        // make sure this is set in case of no subaddress, so following code runs correctly
            if (bytes == 2)
                I2C2->CR1 |= 0x0800;                                    // set the POS bit so NACK applied to the final byte in the two byte read
            I2C_Send7bitAddress(I2C2, addr, I2C_Direction_Receiver);    // send the address and set hardware mode
        } else {                                                        // direction is Tx, or we havent sent the sub and rep start
            I2C_Send7bitAddress(I2C2, addr, I2C_Direction_Transmitter); // send the address and set hardware mode
            if (reg != 0xFF)                                            // 0xFF as subaddress means it will be ignored, in Tx or Rx mode
                index = -1;                                             // send a subaddress
        }
    } else if (SReg_1 & 0x0002) {                                       // we just sent the address - EV6 in ref manual
        // Read SR1,2 to clear ADDR
        __DMB();                                                        // memory fence to control hardware
        if (bytes == 1 && reading && subaddress_sent) {                 // we are receiving 1 byte - EV6_3
            I2C_AcknowledgeConfig(I2C2, DISABLE);                       // turn off ACK
            __DMB();
            (void)I2C2->SR2;                                            // clear ADDR after ACK is turned off
            I2C_GenerateSTOP(I2C2, ENABLE);                             // program the stop
            final_stop = 1;
            I2C_ITConfig(I2C2, I2C_IT_BUF, ENABLE);                     // allow us to have an EV7
        } else {                                                        // EV6 and EV6_1
            (void)I2C2->SR2;                                            // clear the ADDR here
            __DMB();
            if (bytes == 2 && reading && subaddress_sent) {             // rx 2 bytes - EV6_1
                I2C_AcknowledgeConfig(I2C2, DISABLE);                   // turn off ACK
                I2C_ITConfig(I2C2, I2C_IT_BUF, DISABLE);                // disable TXE to allow the buffer to fill
            } else if (bytes == 3 && reading && subaddress_sent)        // rx 3 bytes
                I2C_ITConfig(I2C2, I2C_IT_BUF, DISABLE);                // make sure RXNE disabled so we get a BTF in two bytes time
            else                                                        // receiving greater than three bytes, sending subaddress, or transmitting
                I2C_ITConfig(I2C2, I2C_IT_BUF, ENABLE);
        }
    } else if (SReg_1 & 0x004) {                                        // Byte transfer finished - EV7_2, EV7_3 or EV8_2
        final_stop = 1;
        if (reading && subaddress_sent) {                               // EV7_2, EV7_3
            if (bytes > 2) {                                            // EV7_2
                I2C_AcknowledgeConfig(I2C2, DISABLE);                   // turn off ACK
                read_p[index++] = (uint8_t)I2C2->DR;                    // read data N-2
                I2C_GenerateSTOP(I2C2, ENABLE);                         // program the Stop
                final_stop = 1;                                         // required to fix hardware
                read_p[index++] = (uint8_t)I2C2->DR;                    // read data N - 1
                I2C_ITConfig(I2C2, I2C_IT_BUF, ENABLE);                 // enable TXE to allow the final EV7
            } else {                                                    // EV7_3
                if (final_stop)
                    I2C_GenerateSTOP(I2C2, ENABLE);                     // program the Stop
                else
                    I2C_GenerateSTART(I2C2, ENABLE);                    // program a rep start
                read_p[index++] = (uint8_t)I2C2->DR;                    // read data N - 1
                read_p[index++] = (uint8_t)I2C2->DR;                    // read data N
                index++;                                                // to show job completed
            }
        } else {                                                        // EV8_2, which may be due to a subaddress sent or a write completion
            if (subaddress_sent || (writing)) {
                if (final_stop)
                    I2C_GenerateSTOP(I2C2, ENABLE);                     // program the Stop
                else
                    I2C_GenerateSTART(I2C2, ENABLE);                    // program a rep start
                index++;                                                // to show that the job is complete
            } else {                                                    // We need to send a subaddress
                I2C_GenerateSTART(I2C2, ENABLE);                        // program the repeated Start
                subaddress_sent = 1;                                    // this is set back to zero upon completion of the current task
            }
        }
        // we must wait for the start to clear, otherwise we get constant BTF
        while (I2C2->CR1 & 0x0100) { ; }
    } else if (SReg_1 & 0x0040) {                                       // Byte received - EV7
        read_p[index++] = (uint8_t)I2C2->DR;
        if (bytes == (index + 3))
            I2C_ITConfig(I2C2, I2C_IT_BUF, DISABLE);                    // disable TXE to allow the buffer to flush so we can get an EV7_2
        if (bytes == index)                                             // We have completed a final EV7
            index++;                                                    // to show job is complete
    } else if (SReg_1 & 0x0080) {                                       // Byte transmitted EV8 / EV8_1
        if (index != -1) {                                              // we dont have a subaddress to send
            I2C2->DR = write_p[index++];
            if (bytes == index)                                         // we have sent all the data
                I2C_ITConfig(I2C2, I2C_IT_BUF, DISABLE);                // disable TXE to allow the buffer to flush
        } else {
            index++;
            I2C2->DR = reg;                                             // send the subaddress
            if (reading || !bytes)                                      // if receiving or sending 0 bytes, flush now
                I2C_ITConfig(I2C2, I2C_IT_BUF, DISABLE);                // disable TXE to allow the buffer to flush
        }
    }
    if (index == bytes + 1) {                                           // we have completed the current job
        subaddress_sent = 0;                                            // reset this here
        if (final_stop)                                                 // If there is a final stop and no more jobs, bus is inactive, disable interrupts to prevent BTF
            I2C_ITConfig(I2C2, I2C_IT_EVT | I2C_IT_ERR, DISABLE);       // Disable EVT and ERR interrupts while bus inactive
        busy = 0;
    }
}

void i2cInit()
{
	NVIC_InitTypeDef nvic;
	I2C_InitTypeDef i2c;

	GPIO_InitTypeDef  GPIO_InitStructure;

	//Enable the i2c
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
	//Reset the Peripheral
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, ENABLE);
	RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, DISABLE);

	//Enable the GPIOs for the SCL/SDA Pins
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	//Configure and initialize the GPIOs
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//Connect GPIO pins to peripheral
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_I2C2);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_I2C2);

	// clock out stuff to make sure slaves arent stuck
	// This will also configure GPIO as AF_OD at the end
	//i2cUnstick();

	// Init I2C peripheral
	I2C_DeInit(I2C2);
	I2C_StructInit(&i2c);

	I2C_ITConfig(I2C2, I2C_IT_EVT | I2C_IT_ERR, DISABLE);               // Enable EVT and ERR interrupts - they are enabled by the first request
	i2c.I2C_Mode = I2C_Mode_I2C;
	i2c.I2C_DutyCycle = I2C_DutyCycle_2;
	i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	i2c.I2C_Ack = I2C_Ack_Enable;
	i2c.I2C_OwnAddress1 = 0;
	i2c.I2C_ClockSpeed =  100000;
	I2C_Init(I2C2, &i2c);

	I2C_Cmd(I2C2, ENABLE);

	// I2C ER Interrupt
	nvic.NVIC_IRQChannel = I2C2_ER_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0;
	nvic.NVIC_IRQChannelSubPriority = 2;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);

	// I2C EV Interrupt
	nvic.NVIC_IRQChannel = I2C2_EV_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0;
	nvic.NVIC_IRQChannelSubPriority = 2;
	NVIC_Init(&nvic);
}

uint16_t i2cGetErrorCounter(void)
{
    return i2cErrorCount;
}

void I2C2_ER_IRQHandler(void)
{
    i2c_er_handler();
}

void I2C2_EV_IRQHandler(void)
{
    i2c_ev_handler();
}
