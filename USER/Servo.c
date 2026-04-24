#include "stm32f10x.h"                  // Device header
#include "Servo.h"
#include "delay.h"
 
#define PCA9685_ADDRESS 0x40 
 
void PCA9685_Write(uint8_t reg, uint8_t data) 
{
    // ??????
    I2C_GenerateSTART(I2C2, ENABLE);
    
    // ?? EV5,??????????
    while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));
 
    // ?? PCA9685 ??
    I2C_Send7bitAddress(I2C2, PCA9685_ADDRESS, I2C_Direction_Transmitter);
    
    // ?? EV6,???????
    while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
 
    // ???????
    I2C_SendData(I2C2, reg);
    
    // ?? EV8_2,??????
    while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING));
    
    // ????
    I2C_SendData(I2C2, data);
    
    // ????????
    while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
    
    // ??????
    I2C_GenerateSTOP(I2C2, ENABLE);
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
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
    // 配置I2C2 SCL (PB6) 和SDA (PB7)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	I2C_InitTypeDef I2C_InitStructure;
    // ?? I2C ??
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 1000; // 100kHz  
    // 初始化 I2C2
    I2C_Init(I2C2, &I2C_InitStructure);
    
    //使能 I2C2
    I2C_Cmd(I2C2, ENABLE);
	
    PCA9685_SetPWMFreq(50.0f); // 设置PCA9685的PWM频率
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
