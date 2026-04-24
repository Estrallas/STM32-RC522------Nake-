#include "stm32f10x.h"                  // Device header
#include "Servo.h"
#include "delay.h"
 
#define PCA9685_ADDRESS 0x80 
 
int PCA9685_Write(uint8_t reg, uint8_t data) 
{
    // Non-blocking with timeout to avoid hang if I2C bus/device absent
    uint32_t timeout;

    I2C_GenerateSTART(I2C1, ENABLE);
    // wait EV5
    timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT)) {
        if (--timeout == 0) { I2C_GenerateSTOP(I2C1, ENABLE); return -1; }
    }

    // send address
    I2C_Send7bitAddress(I2C1, PCA9685_ADDRESS, I2C_Direction_Transmitter);
    // wait EV6
    timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
        if (--timeout == 0) { I2C_GenerateSTOP(I2C1, ENABLE); return -1; }
    }

    // send reg
    I2C_SendData(I2C1, reg);
    // wait EV8_2
    timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTING)) {
        if (--timeout == 0) { I2C_GenerateSTOP(I2C1, ENABLE); return -1; }
    }

    // send data
    I2C_SendData(I2C1, data);
    // wait EV8_2 transmitted
    timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
        if (--timeout == 0) { I2C_GenerateSTOP(I2C1, ENABLE); return -1; }
    }

    // stop
    I2C_GenerateSTOP(I2C1, ENABLE);
    return 0;
}
 
void PCA9685_SetPWMFreq(float freq) 
{
    uint8_t prescale_val = (uint8_t)(25000000 / (4096 * freq)) - 1;
 
    // ?????????PRE_SCALE
    PCA9685_Write(0x00, 0x10); // MODE1 ???,???????
    
    // ?? PRE_SCALE
    PCA9685_Write(0xFE, prescale_val);
 
    // ??????,??????
    PCA9685_Write(0x00, 0x80); // MODE1 ???,????
}
 
void PCA9685_Init(void)
{
    // ????
     // Use I2C1 (PB6=SCL, PB7=SDA) so it doesn't conflict with OLED bit-banged I2C on PB8/PB9
     RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
     RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

     GPIO_InitTypeDef GPIO_InitStructure;
     // I2C1 SCL (PB6) and SDA (PB7)
     GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
     GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
     GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
     GPIO_Init(GPIOB, &GPIO_InitStructure);

     I2C_InitTypeDef I2C_InitStructure;
     // I2C configuration
     I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
     I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
     I2C_InitStructure.I2C_OwnAddress1 = 0x00;
     I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
     I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
     I2C_InitStructure.I2C_ClockSpeed = 100000; // 100kHz
     // init I2C1
     I2C_Init(I2C1, &I2C_InitStructure);
    
     // enable I2C1
     I2C_Cmd(I2C1, ENABLE);
	
    PCA9685_SetPWMFreq(50.0f); // ����PCA9685��PWMƵ��
		PCA9685_Write(0x00, 0x00); // MODE1 ???,??????
    PCA9685_Write(0x01, 0x04); // MODE2 ???,????????
}
 
void PCA9685_SetPWM(uint8_t channel, uint16_t on, uint16_t off) 
{
    PCA9685_Write(0x06 + 4 * channel, on & 0xFF);
    PCA9685_Write(0x07 + 4 * channel, on >> 8);
    PCA9685_Write(0x08 + 4 * channel, off & 0xFF);
    PCA9685_Write(0x09 + 4 * channel, off >> 8);
}
 
uint16_t AngleToPWM(float angle) 
{
    // angle in degrees (0 to 270)
    float pwm = ((angle / 270.0) * (512 - 102)) + 102;
    return (uint16_t)pwm;
}
 
void PCA9685_SetAngle(uint8_t channel, float angle) 
{
    uint16_t off = AngleToPWM(angle);
    PCA9685_SetPWM(channel, 0, off);
}
