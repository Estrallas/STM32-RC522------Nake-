/**
  * @file       pca9685_device.h
  * @author     拉咯比哩
  * @version    V1.0.0
  * @date       20260416
  * @brief      PCA9685 舵机驱动，基于 STM32 HAL 库
  * 
  * <h2><center>&copy;此文件版权归【拉咯比哩】所有.</center></h2>
  */

#ifndef _PCA9685_DEVICE_H_
#define _PCA9685_DEVICE_H_

#include "project_config.h"
#include "stm32f1xx_hal.h"


#define Servo_Open 90.0f
#define Servo_Close 0.0f

/// @brief          设备号枚举
///
/// @note
typedef enum
{
    emPca9685DevNum0        = 0,
    emPca9685DevNum1,
}
emPca9685DevNumTdf;

/// @brief          静态参数定义
///
/// @note
typedef struct
{
    I2C_HandleTypeDef   *pstI2cHandle;                  // I2C 句柄指针
    uint16_t            usDevAddr;                      // 设备地址，需左移 1 位，如 0x40 -> 0x80
    uint16_t            usServoMinPulseUs;              // 舵机最小脉宽，单位 us
    uint16_t            usServoMaxPulseUs;              // 舵机最大脉宽，单位 us
}
stPca9685StaticParamTdf;

/// @brief          运行参数定义
///
/// @note
typedef struct
{
    uint16_t            usPwmFreqHz;                    // PWM 频率，单位 Hz
}
stPca9685RunningParamTdf;

/// @brief          结构参数定义
///
/// @note
typedef struct
{
    stPca9685StaticParamTdf     stStaticParam;
    stPca9685RunningParamTdf    stRunningParam;
}
stPca9685DeviceParamTdf;


void vPca9685DeviceInit(stPca9685StaticParamTdf *pstInit, emPca9685DevNumTdf emDevNum);
void vPca9685SetPwmFreq(uint16_t usFreqHz, emPca9685DevNumTdf emDevNum);
void vPca9685SetPwmByTick(uint8_t ucChannel, uint16_t usOnTick, uint16_t usOffTick, emPca9685DevNumTdf emDevNum);
void vPca9685SetServoPulseUs(uint8_t ucChannel, uint16_t usPulseUs, emPca9685DevNumTdf emDevNum);
void vPca9685SetServoAngle(float fAngle, uint8_t ucChannel, emPca9685DevNumTdf emDevNum);

#endif
