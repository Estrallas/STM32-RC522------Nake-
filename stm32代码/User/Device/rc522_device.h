/**
  * @file       rc522_device.h
  * @author     拉咯比哩
  * @version    V1.0.0
  * @date       20260418
  * @brief      RC522 驱动，基于 STM32 HAL 库
  *
  * <h2><center>&copy;此文件版权归【拉咯比哩】所有.</center></h2>
  */

#ifndef _RC522_DEVICE_H_
#define _RC522_DEVICE_H_

#include "project_config.h"
#include "stm32f1xx_hal.h"

/// @brief          设备号枚举
///
/// @note
typedef enum
{
    emRc522DevNum0          = 0,
    emRc522DevNum1,
}
emRc522DevNumTdf;

/// @brief          静态参数定义
///
/// @note
typedef struct
{
    SPI_HandleTypeDef   *pstSpiHandle;                  // SPI 句柄指针
    GPIO_TypeDef        *pstNssPort;                    // NSS 引脚端口
    uint16_t            usNssPin;                       // NSS 引脚
    GPIO_TypeDef        *pstRstPort;                    // RST 引脚端口
    uint16_t            usRstPin;                       // RST 引脚
}
stRc522StaticParamTdf;

/// @brief          运行参数定义
///
/// @note
typedef struct
{
    uint8_t             aucUid[10];                     // 最近一次读到的 UID
    uint8_t             ucUidLen;                       // UID 长度
}
stRc522RunningParamTdf;

/// @brief          结构参数定义
///
/// @note
typedef struct
{
    stRc522StaticParamTdf      stStaticParam;
    stRc522RunningParamTdf     stRunningParam;
}
stRc522DeviceParamTdf;

void vRc522DeviceInit(stRc522StaticParamTdf *pstInit, emRc522DevNumTdf emDevNum);
uint8_t ucRc522ReadUid(uint8_t *pucUid, uint8_t *pucUidLen, emRc522DevNumTdf emDevNum);
uint8_t ucRc522ReadUidHex(char *pcUidHex, uint16_t usMaxLen, emRc522DevNumTdf emDevNum);
uint8_t ucRc522UidHexToKey32(const char *pcUidHex, uint32_t *pulUidKey);
void vRc522UidToString(const uint8_t *c_pucUid, uint8_t ucUidLen, char *pcOut, uint16_t usMaxLen);

#endif