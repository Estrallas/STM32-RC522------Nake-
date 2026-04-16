#include "stm32f10x.h"                  // Device header
#include "stm32f10x_gpio.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_rcc.h"
#include "Servo.h"
#include "delay.h"
 
// I2C_Send7bitAddress expects address in bits[7:1], so 0x40 must be left-shifted.
#define PCA9685_ADDRESS (0x40 << 1)
#define PCA9685_I2C I2C1
#define PCA9685_I2C_RCC RCC_APB1Periph_I2C1
#define PCA9685_GPIO_RCC RCC_APB2Periph_GPIOB
#define PCA9685_GPIO_PORT GPIOB
#define PCA9685_SCL_PIN GPIO_Pin_8
#define PCA9685_SDA_PIN GPIO_Pin_9
#define PCA9685_I2C_SPEED 100000U
#define PCA9685_I2C_TIMEOUT 60000U

#define PCA9685_MODE1_REG 0x00
#define PCA9685_MODE2_REG 0x01
#define PCA9685_PRESCALE_REG 0xFE

static void PCA9685_I2C_Config(void)
{
    I2C_InitTypeDef I2C_InitStructure;

    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = PCA9685_I2C_SPEED;
    I2C_Init(PCA9685_I2C, &I2C_InitStructure);
    I2C_Cmd(PCA9685_I2C, ENABLE);
}

static uint8_t PCA9685_WaitEvent(uint32_t event)
{
    uint32_t timeout = PCA9685_I2C_TIMEOUT;
    while (!I2C_CheckEvent(PCA9685_I2C, event))
    {
        if (timeout-- == 0U)
        {
            return 0U;
        }
    }
    return 1U;
}

static uint8_t PCA9685_WaitFlagClear(uint32_t flag)
{
    uint32_t timeout = PCA9685_I2C_TIMEOUT;
    while (I2C_GetFlagStatus(PCA9685_I2C, flag) == SET)
    {
        if (timeout-- == 0U)
        {
            return 0U;
        }
    }
    return 1U;
}

static void PCA9685_I2C_BusReset(void)
{
    I2C_GenerateSTOP(PCA9685_I2C, ENABLE);
    I2C_Cmd(PCA9685_I2C, DISABLE);
    I2C_SoftwareResetCmd(PCA9685_I2C, ENABLE);
    delay_us(10);
    I2C_SoftwareResetCmd(PCA9685_I2C, DISABLE);
    PCA9685_I2C_Config();
}

static uint8_t PCA9685_WriteReg(uint8_t reg, uint8_t data)
{
    if (!PCA9685_WaitFlagClear(I2C_FLAG_BUSY))
    {
        PCA9685_I2C_BusReset();
        if (!PCA9685_WaitFlagClear(I2C_FLAG_BUSY))
        {
            return 0U;
        }
    }

    I2C_GenerateSTART(PCA9685_I2C, ENABLE);
    if (!PCA9685_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT))
    {
        goto fail;
    }

    I2C_Send7bitAddress(PCA9685_I2C, PCA9685_ADDRESS, I2C_Direction_Transmitter);
    if (!PCA9685_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        goto fail;
    }

    I2C_SendData(PCA9685_I2C, reg);
    if (!PCA9685_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
        goto fail;
    }

    I2C_SendData(PCA9685_I2C, data);
    if (!PCA9685_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
        goto fail;
    }

    I2C_GenerateSTOP(PCA9685_I2C, ENABLE);
    return 1U;

fail:
    I2C_GenerateSTOP(PCA9685_I2C, ENABLE);
    PCA9685_I2C_BusReset();
    return 0U;
}
 
void PCA9685_Write(uint8_t reg, uint8_t data) 
{
    (void)PCA9685_WriteReg(reg, data);
}
 
void PCA9685_SetPWMFreq(float freq) 
{
    uint8_t prescale_val;

    if (freq < 24.0f)
    {
        freq = 24.0f;
    }
    else if (freq > 1526.0f)
    {
        freq = 1526.0f;
    }

    prescale_val = (uint8_t)((25000000.0f / (4096.0f * freq)) + 0.5f) - 1U;

    // Enter sleep before writing prescale.
    PCA9685_Write(PCA9685_MODE1_REG, 0x10);
    PCA9685_Write(PCA9685_PRESCALE_REG, prescale_val);

    // Wake up and restart oscillator.
    PCA9685_Write(PCA9685_MODE1_REG, 0x20);
    delay_ms(1);
    PCA9685_Write(PCA9685_MODE1_REG, 0xA0);
}
 
void PCA9685_Init(void)
{
    RCC_APB2PeriphClockCmd(PCA9685_GPIO_RCC | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(PCA9685_I2C_RCC, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);

		GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = PCA9685_SCL_PIN | PCA9685_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(PCA9685_GPIO_PORT, &GPIO_InitStructure);
    GPIO_SetBits(PCA9685_GPIO_PORT, PCA9685_SCL_PIN | PCA9685_SDA_PIN);

        PCA9685_I2C_Config();

    PCA9685_I2C_BusReset();

        PCA9685_Write(PCA9685_MODE2_REG, 0x04); // Totem-pole output
    PCA9685_SetPWMFreq(50.0f);
        PCA9685_Write(PCA9685_MODE1_REG, 0x20);
}
 
void PCA9685_SetPWM(uint8_t channel, uint16_t on, uint16_t off) 
{
    if (channel > 15)
    {
        return;
    }

    PCA9685_Write(0x06 + 4 * channel, on & 0xFF);
    PCA9685_Write(0x07 + 4 * channel, on >> 8);
    PCA9685_Write(0x08 + 4 * channel, off & 0xFF);
    PCA9685_Write(0x09 + 4 * channel, off >> 8);
}
 
uint16_t AngleToPWM(float angle) 
{
    // Most servos use about 0~180 degrees.
    if (angle < 0)
    {
        angle = 0;
    }
    else if (angle > 180)
    {
        angle = 180;
    }

    float pwm = ((angle / 180.0f) * (512 - 102)) + 102;
    return (uint16_t)pwm;
}
 
void PCA9685_SetAngle(uint8_t channel, float angle) 
{
    uint16_t off = AngleToPWM(angle);
    PCA9685_SetPWM(channel, 0, off);
}
