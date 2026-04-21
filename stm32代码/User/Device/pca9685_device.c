/**
  * @file       pca9685_device.c
  * @author     拉咯比哩
  * @version    V1.0.0
  * @date       20260416
  * @brief      PCA9685 舵机驱动，基于 STM32 HAL 库
  *
  * <h2><center>&copy;此文件版权归【拉咯比哩】所有.</center></h2>
  */

#include "pca9685_device.h"
#include "string.h"

#define PCA9685_REG_MODE1                            0x00u
#define PCA9685_REG_MODE2                            0x01u
#define PCA9685_REG_LED0_ON_L                        0x06u
#define PCA9685_REG_PRE_SCALE                        0xFEu

#define PCA9685_MODE1_RESTART                        0x80u
#define PCA9685_MODE1_SLEEP                          0x10u
#define PCA9685_MODE1_AI                             0x20u
#define PCA9685_MODE1_ALLCALL                        0x01u
#define PCA9685_MODE2_OUTDRV                         0x04u

#define PCA9685_INTERNAL_OSC_HZ                      25000000u
#define PCA9685_TICKS_PER_PERIOD                     4096u

stPca9685DeviceParamTdf astPca9685DeviceParam[PCA9685_DEV_NUM];


/// @brief      IIC 写 1 字节
///
/// @param      ucRegAddr    ：寄存器地址
///             ucValue      ：寄存器值
///             emDevNum     ：设备号
///
/// @note
static void s_vPca9685WriteOneByteReg(uint8_t ucRegAddr, uint8_t ucValue, emPca9685DevNumTdf emDevNum)
{
    HAL_I2C_Mem_Write(astPca9685DeviceParam[emDevNum].stStaticParam.pstI2cHandle,
                      astPca9685DeviceParam[emDevNum].stStaticParam.usDevAddr,
                      ucRegAddr,
                      I2C_MEMADD_SIZE_8BIT,
                      &ucValue,
                      1u,
                      100u);
}

/// @brief      IIC 读 1 字节
///
/// @param      ucRegAddr    ：寄存器地址
///             pucValue     ：读回数据指针
///             emDevNum     ：设备号
///
/// @note
static void s_vPca9685ReadOneByteReg(uint8_t ucRegAddr, uint8_t *pucValue, emPca9685DevNumTdf emDevNum)
{
    HAL_I2C_Mem_Read(astPca9685DeviceParam[emDevNum].stStaticParam.pstI2cHandle,
                     astPca9685DeviceParam[emDevNum].stStaticParam.usDevAddr,
                     ucRegAddr,
                     I2C_MEMADD_SIZE_8BIT,
                     pucValue,
                     1u,
                     100u);
}

/// @brief      设置 PWM 频率
///
/// @param      usFreqHz     ：频率，单位 Hz
///             emDevNum     ：设备号
///
/// @note
void vPca9685SetPwmFreq(uint16_t usFreqHz, emPca9685DevNumTdf emDevNum)
{
    uint8_t ucOldMode;
    uint8_t ucSleepMode;
    uint8_t ucPreScale;
    uint32_t ulPreScale;

    if(usFreqHz == 0u)
    {
        return;
    }

    ulPreScale = (PCA9685_INTERNAL_OSC_HZ + ((uint32_t)usFreqHz * PCA9685_TICKS_PER_PERIOD / 2u)) /
                 ((uint32_t)usFreqHz * PCA9685_TICKS_PER_PERIOD);

    if(ulPreScale == 0u)
    {
        ulPreScale = 1u;
    }

    ulPreScale -= 1u;
    if(ulPreScale > 255u)
    {
        ulPreScale = 255u;
    }

    ucPreScale = (uint8_t)ulPreScale;

    s_vPca9685ReadOneByteReg(PCA9685_REG_MODE1, &ucOldMode, emDevNum);
    ucSleepMode = (uint8_t)((ucOldMode & (uint8_t)(~PCA9685_MODE1_RESTART)) | PCA9685_MODE1_SLEEP);

    s_vPca9685WriteOneByteReg(PCA9685_REG_MODE1, ucSleepMode, emDevNum);
    s_vPca9685WriteOneByteReg(PCA9685_REG_PRE_SCALE, ucPreScale, emDevNum);
    s_vPca9685WriteOneByteReg(PCA9685_REG_MODE1, ucOldMode, emDevNum);

    HAL_Delay(1u);

    s_vPca9685WriteOneByteReg(PCA9685_REG_MODE1, (uint8_t)(ucOldMode | PCA9685_MODE1_RESTART), emDevNum);

    astPca9685DeviceParam[emDevNum].stRunningParam.usPwmFreqHz = usFreqHz;
}

/// @brief      设置某一通道 PWM Tick
///
/// @param      ucChannel    ：通道号，范围 0 ~ 15
///             usOnTick     ：高电平起始 Tick，范围 0 ~ 4095
///             usOffTick    ：高电平结束 Tick，范围 0 ~ 4095
///             emDevNum     ：设备号
///
/// @note
void vPca9685SetPwmByTick(uint8_t ucChannel, uint16_t usOnTick, uint16_t usOffTick, emPca9685DevNumTdf emDevNum)
{
    uint8_t ucRegAddr;

    if(ucChannel >= PCA9685_CHANNEL_NUM)
    {
        return;
    }

    usOnTick &= 0x0FFFu;
    usOffTick &= 0x0FFFu;

    ucRegAddr = (uint8_t)(PCA9685_REG_LED0_ON_L + (uint8_t)(4u * ucChannel));

    // 逐寄存器写入，避免 AI 位异常导致多字节写失效
    s_vPca9685WriteOneByteReg((uint8_t)(ucRegAddr + 0u), (uint8_t)(usOnTick & 0xFFu), emDevNum);
    s_vPca9685WriteOneByteReg((uint8_t)(ucRegAddr + 1u), (uint8_t)((usOnTick >> 8) & 0x0Fu), emDevNum);
    s_vPca9685WriteOneByteReg((uint8_t)(ucRegAddr + 2u), (uint8_t)(usOffTick & 0xFFu), emDevNum);
    s_vPca9685WriteOneByteReg((uint8_t)(ucRegAddr + 3u), (uint8_t)((usOffTick >> 8) & 0x0Fu), emDevNum);
}

/// @brief      设置舵机脉宽
///
/// @param      ucChannel    ：通道号，范围 0 ~ 15
///             usPulseUs    ：脉宽，单位 us
///             emDevNum     ：设备号
///
/// @note
void vPca9685SetServoPulseUs(uint8_t ucChannel, uint16_t usPulseUs, emPca9685DevNumTdf emDevNum)
{
    uint32_t ulTick;
    uint32_t ulFreq;

    if(ucChannel >= PCA9685_CHANNEL_NUM)
    {
        return;
    }

    if(usPulseUs < astPca9685DeviceParam[emDevNum].stStaticParam.usServoMinPulseUs)
    {
        usPulseUs = astPca9685DeviceParam[emDevNum].stStaticParam.usServoMinPulseUs;
    }

    if(usPulseUs > astPca9685DeviceParam[emDevNum].stStaticParam.usServoMaxPulseUs)
    {
        usPulseUs = astPca9685DeviceParam[emDevNum].stStaticParam.usServoMaxPulseUs;
    }

    ulFreq = astPca9685DeviceParam[emDevNum].stRunningParam.usPwmFreqHz;
    if(ulFreq == 0u)
    {
        return;
    }

    ulTick = ((uint32_t)usPulseUs * ulFreq * PCA9685_TICKS_PER_PERIOD + 500000u) / 1000000u;
    if(ulTick > 4095u)
    {
        ulTick = 4095u;
    }

    vPca9685SetPwmByTick(ucChannel, 0u, (uint16_t)ulTick, emDevNum);
}

/// @brief      设置舵机角度
///
/// @param      fAngle       ：角度，范围 0 ~ PCA9685_SERVO_ANGLE_MAX
///             ucChannel    ：通道号，范围 0 ~ 15
///             emDevNum     ：设备号
///
/// @note
void vPca9685SetServoAngle(float fAngle, uint8_t ucChannel, emPca9685DevNumTdf emDevNum)
{
    float fPulseUs;
    float fMinPulse;
    float fMaxPulse;

    if(fAngle < 0.0f)
    {
        fAngle = 0.0f;
    }

    if(fAngle > PCA9685_SERVO_ANGLE_MAX)
    {
        fAngle = PCA9685_SERVO_ANGLE_MAX;
    }

    fMinPulse = (float)astPca9685DeviceParam[emDevNum].stStaticParam.usServoMinPulseUs;
    fMaxPulse = (float)astPca9685DeviceParam[emDevNum].stStaticParam.usServoMaxPulseUs;

    fPulseUs = fMinPulse + (fAngle / PCA9685_SERVO_ANGLE_MAX) * (fMaxPulse - fMinPulse);

    vPca9685SetServoPulseUs(ucChannel, (uint16_t)(fPulseUs + 0.5f), emDevNum);
}

/// @brief      PCA9685 初始化
///
/// @param      pstInit      ：初始化参数结构体首地址
///             emDevNum     ：设备号
///
/// @note
void vPca9685DeviceInit(stPca9685StaticParamTdf *pstInit, emPca9685DevNumTdf emDevNum)
{
    uint8_t ucMode1;

    // 1. 初始化静态参数
    memcpy(&astPca9685DeviceParam[emDevNum].stStaticParam,
           pstInit,
           sizeof(stPca9685StaticParamTdf) / sizeof(uint8_t));

    // 2. 初始化运行参数
    astPca9685DeviceParam[emDevNum].stRunningParam.usPwmFreqHz = 0u;

    // 3. MODE 寄存器初始化
    s_vPca9685WriteOneByteReg(PCA9685_REG_MODE2, PCA9685_MODE2_OUTDRV, emDevNum);
    ucMode1 = (uint8_t)(PCA9685_MODE1_ALLCALL | PCA9685_MODE1_AI);
    s_vPca9685WriteOneByteReg(PCA9685_REG_MODE1, ucMode1, emDevNum);

    HAL_Delay(1u);

    // 4. 默认设置 50Hz 舵机频率
    vPca9685SetPwmFreq(PCA9685_SERVO_DEFAULT_FREQ_HZ, emDevNum);
}
