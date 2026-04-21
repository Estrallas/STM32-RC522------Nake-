#include "lcd1602_i2c_test.h"

#define LCD1602_I2C_ADDR_7BIT               0x27u
#define LCD1602_I2C_ADDR_HAL                ((uint16_t)(LCD1602_I2C_ADDR_7BIT << 1u))

#define LCD1602_RS_BIT                      0x01u
#define LCD1602_EN_BIT                      0x04u
#define LCD1602_BACKLIGHT_BIT               0x08u

#define LCD1602_CMD_FUNCTION_SET_4BIT_2LINE 0x28u
#define LCD1602_CMD_DISPLAY_OFF             0x08u
#define LCD1602_CMD_DISPLAY_ON              0x0Cu
#define LCD1602_CMD_ENTRY_MODE              0x06u
#define LCD1602_CMD_CLEAR                   0x01u
#define LCD1602_CMD_DDRAM_LINE1             0x80u

static void s_vLcd1602WriteRaw(I2C_HandleTypeDef *pstI2cHandle, uint8_t ucData)
{
    HAL_I2C_Master_Transmit(pstI2cHandle, LCD1602_I2C_ADDR_HAL, &ucData, 1u, 20u);
}

static void s_vLcd1602PulseEnable(I2C_HandleTypeDef *pstI2cHandle, uint8_t ucData)
{
    s_vLcd1602WriteRaw(pstI2cHandle, (uint8_t)(ucData | LCD1602_EN_BIT));
    HAL_Delay(1u);
    s_vLcd1602WriteRaw(pstI2cHandle, (uint8_t)(ucData & (uint8_t)(~LCD1602_EN_BIT)));
    HAL_Delay(1u);
}

static void s_vLcd1602Write4Bit(I2C_HandleTypeDef *pstI2cHandle, uint8_t ucDataNibble, uint8_t ucIsData)
{
    uint8_t ucBusData;

    ucBusData = (uint8_t)(ucDataNibble & 0xF0u);
    ucBusData |= LCD1602_BACKLIGHT_BIT;
    if(ucIsData != 0u)
    {
        ucBusData |= LCD1602_RS_BIT;
    }

    s_vLcd1602WriteRaw(pstI2cHandle, ucBusData);
    s_vLcd1602PulseEnable(pstI2cHandle, ucBusData);
}

static void s_vLcd1602WriteByte(I2C_HandleTypeDef *pstI2cHandle, uint8_t ucData, uint8_t ucIsData)
{
    s_vLcd1602Write4Bit(pstI2cHandle, ucData, ucIsData);
    s_vLcd1602Write4Bit(pstI2cHandle, (uint8_t)(ucData << 4u), ucIsData);
}

static void s_vLcd1602Init(I2C_HandleTypeDef *pstI2cHandle)
{
    HAL_Delay(50u);

    s_vLcd1602Write4Bit(pstI2cHandle, 0x30u, 0u);
    HAL_Delay(5u);
    s_vLcd1602Write4Bit(pstI2cHandle, 0x30u, 0u);
    HAL_Delay(1u);
    s_vLcd1602Write4Bit(pstI2cHandle, 0x30u, 0u);
    HAL_Delay(1u);

    s_vLcd1602Write4Bit(pstI2cHandle, 0x20u, 0u);
    HAL_Delay(1u);

    s_vLcd1602WriteByte(pstI2cHandle, LCD1602_CMD_FUNCTION_SET_4BIT_2LINE, 0u);
    s_vLcd1602WriteByte(pstI2cHandle, LCD1602_CMD_DISPLAY_OFF, 0u);
    s_vLcd1602WriteByte(pstI2cHandle, LCD1602_CMD_CLEAR, 0u);
    HAL_Delay(2u);
    s_vLcd1602WriteByte(pstI2cHandle, LCD1602_CMD_ENTRY_MODE, 0u);
    s_vLcd1602WriteByte(pstI2cHandle, LCD1602_CMD_DISPLAY_ON, 0u);
}

static void s_vLcd1602WriteString(I2C_HandleTypeDef *pstI2cHandle, const char *pcString)
{
    const char *pcCursor;

    if(pcString == NULL)
    {
        return;
    }

    pcCursor = pcString;
    while(*pcCursor != '\0')
    {
        s_vLcd1602WriteByte(pstI2cHandle, (uint8_t)(*pcCursor), 1u);
        pcCursor++;
    }
}

void vLcd1602I2cTestShowHelloWorld(I2C_HandleTypeDef *pstI2cHandle)
{
    if(pstI2cHandle == NULL)
    {
        return;
    }

    if(HAL_I2C_IsDeviceReady(pstI2cHandle, LCD1602_I2C_ADDR_HAL, 2u, 20u) != HAL_OK)
    {
        return;
    }

    s_vLcd1602Init(pstI2cHandle);
    s_vLcd1602WriteByte(pstI2cHandle, LCD1602_CMD_DDRAM_LINE1, 0u);
    s_vLcd1602WriteString(pstI2cHandle, "HelloWorld");
}
