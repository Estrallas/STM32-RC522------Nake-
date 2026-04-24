/**
  * @file       rc522_device.c
  * @author     拉咯比哩
  * @version    V1.0.0
  * @date       20260418
  * @brief      RC522 驱动，基于 STM32 HAL 库
  *
  * <h2><center>&copy;此文件版权归【拉咯比哩】所有.</center></h2>
  */

#include "rc522_device.h"
#include "string.h"

#define RC522_STATUS_OK                                 0u
#define RC522_STATUS_ERR                                1u
#define RC522_STATUS_NO_TAG                             2u

#define RC522_CMD_IDLE                                  0x00u
#define RC522_CMD_AUTHENT                               0x0Eu
#define RC522_CMD_RECEIVE                               0x08u
#define RC522_CMD_TRANSMIT                              0x04u
#define RC522_CMD_TRANSCEIVE                            0x0Cu
#define RC522_CMD_RESETPHASE                            0x0Fu

#define RC522_PICC_REQIDL                               0x26u
#define RC522_PICC_ANTICOLL                             0x93u

#define RC522_REG_COMMAND                               0x01u
#define RC522_REG_COMMIEN                               0x02u
#define RC522_REG_COMMIRQ                               0x04u
#define RC522_REG_ERROR                                 0x06u
#define RC522_REG_STATUS2                               0x08u
#define RC522_REG_FIFO_DATA                             0x09u
#define RC522_REG_FIFO_LEVEL                            0x0Au
#define RC522_REG_CONTROL                               0x0Cu
#define RC522_REG_BIT_FRAMING                           0x0Du
#define RC522_REG_MODE                                  0x11u
#define RC522_REG_TX_CONTROL                            0x14u
#define RC522_REG_TX_AUTO                               0x15u
#define RC522_REG_T_MODE                                0x2Au
#define RC522_REG_T_PRESCALER                           0x2Bu
#define RC522_REG_T_RELOAD_L                            0x2Du
#define RC522_REG_T_RELOAD_H                            0x2Cu

#define RC522_UID_HEX_MAX_LEN                           3u

stRc522DeviceParamTdf astRc522DeviceParam[RC522_DEV_NUM];

/// @brief      字节转十六进制字符
///
/// @param      ucValue     ：输入字节
///             pcOut       ：输出 2 字符地址
///
/// @note
static void s_vByteToHex(uint8_t ucValue, char *pcOut)
{
    const char c_auHex[] = "0123456789ABCDEF";

    pcOut[0] = c_auHex[(ucValue >> 4) & 0x0Fu];
    pcOut[1] = c_auHex[ucValue & 0x0Fu];
}

/// @brief      单个十六进制字符转 4bit 数值
///
/// @param      cHexChar    ：十六进制字符
///             pucNibble   ：输出 4bit 数值
///
/// @note
static uint8_t s_ucHexCharToNibble(char cHexChar, uint8_t *pucNibble)
{
    if(pucNibble == NULL)
    {
        return FALSE;
    }

    if((cHexChar >= '0') && (cHexChar <= '9'))
    {
        *pucNibble = (uint8_t)(cHexChar - '0');
        return TRUE;
    }

    if((cHexChar >= 'A') && (cHexChar <= 'F'))
    {
        *pucNibble = (uint8_t)(cHexChar - 'A' + 10);
        return TRUE;
    }

    if((cHexChar >= 'a') && (cHexChar <= 'f'))
    {
        *pucNibble = (uint8_t)(cHexChar - 'a' + 10);
        return TRUE;
    }

    return FALSE;
}

/// @brief      控制 RC522 片选
///
/// @param      ucLevel     ：GPIO_PIN_SET 或 GPIO_PIN_RESET
///             emDevNum    ：设备号
///
/// @note
static void s_vRc522SetNss(GPIO_PinState ucLevel, emRc522DevNumTdf emDevNum)
{
    HAL_GPIO_WritePin(astRc522DeviceParam[emDevNum].stStaticParam.pstNssPort,
                      astRc522DeviceParam[emDevNum].stStaticParam.usNssPin,
                      ucLevel);
}

/// @brief      写 RC522 寄存器
///
/// @param      ucAddr      ：寄存器地址
///             ucValue     ：写入值
///             emDevNum    ：设备号
///
/// @note
static void s_vRc522WriteReg(uint8_t ucAddr, uint8_t ucValue, emRc522DevNumTdf emDevNum)
{
    uint8_t aucTx[2];

    aucTx[0] = (uint8_t)((ucAddr << 1) & 0x7Eu);
    aucTx[1] = ucValue;

    s_vRc522SetNss(GPIO_PIN_RESET, emDevNum);
    HAL_SPI_Transmit(astRc522DeviceParam[emDevNum].stStaticParam.pstSpiHandle, aucTx, 2u, 10u);
    s_vRc522SetNss(GPIO_PIN_SET, emDevNum);
}

/// @brief      读 RC522 寄存器
///
/// @param      ucAddr      ：寄存器地址
///             emDevNum    ：设备号
///
/// @note
static uint8_t s_ucRc522ReadReg(uint8_t ucAddr, emRc522DevNumTdf emDevNum)
{
    uint8_t aucTx[2];
    uint8_t aucRx[2];

    aucTx[0] = (uint8_t)(((ucAddr << 1) & 0x7Eu) | 0x80u);
    aucTx[1] = 0x00u;
    aucRx[0] = 0x00u;
    aucRx[1] = 0x00u;

    s_vRc522SetNss(GPIO_PIN_RESET, emDevNum);
    HAL_SPI_TransmitReceive(astRc522DeviceParam[emDevNum].stStaticParam.pstSpiHandle, aucTx, aucRx, 2u, 10u);
    s_vRc522SetNss(GPIO_PIN_SET, emDevNum);

    return aucRx[1];
}

/// @brief      RC522 寄存器置位
///
/// @param      ucReg       ：寄存器地址
///             ucMask      ：掩码
///             emDevNum    ：设备号
///
/// @note
static void s_vRc522SetBitMask(uint8_t ucReg, uint8_t ucMask, emRc522DevNumTdf emDevNum)
{
    uint8_t ucTemp;

    ucTemp = s_ucRc522ReadReg(ucReg, emDevNum);
    s_vRc522WriteReg(ucReg, (uint8_t)(ucTemp | ucMask), emDevNum);
}

/// @brief      RC522 寄存器清位
///
/// @param      ucReg       ：寄存器地址
///             ucMask      ：掩码
///             emDevNum    ：设备号
///
/// @note
static void s_vRc522ClearBitMask(uint8_t ucReg, uint8_t ucMask, emRc522DevNumTdf emDevNum)
{
    uint8_t ucTemp;

    ucTemp = s_ucRc522ReadReg(ucReg, emDevNum);
    s_vRc522WriteReg(ucReg, (uint8_t)(ucTemp & (uint8_t)(~ucMask)), emDevNum);
}

/// @brief      打开天线
///
/// @param      emDevNum    ：设备号
///
/// @note
static void s_vRc522AntennaOn(emRc522DevNumTdf emDevNum)
{
    uint8_t ucTemp;

    ucTemp = s_ucRc522ReadReg(RC522_REG_TX_CONTROL, emDevNum);
    if((ucTemp & 0x03u) == 0u)
    {
        s_vRc522SetBitMask(RC522_REG_TX_CONTROL, 0x03u, emDevNum);
    }
}

/// @brief      RC522 软复位并配置参数
///
/// @param      emDevNum    ：设备号
///
/// @note
static void s_vRc522Reset(emRc522DevNumTdf emDevNum)
{
    s_vRc522WriteReg(RC522_REG_COMMAND, RC522_CMD_RESETPHASE, emDevNum);
    HAL_Delay(2u);

    s_vRc522WriteReg(RC522_REG_T_MODE, 0x8Du, emDevNum);
    s_vRc522WriteReg(RC522_REG_T_PRESCALER, 0x3Eu, emDevNum);
    s_vRc522WriteReg(RC522_REG_T_RELOAD_L, 30u, emDevNum);
    s_vRc522WriteReg(RC522_REG_T_RELOAD_H, 0u, emDevNum);
    s_vRc522WriteReg(RC522_REG_TX_AUTO, 0x40u, emDevNum);
    s_vRc522WriteReg(RC522_REG_MODE, 0x3Du, emDevNum);

    s_vRc522AntennaOn(emDevNum);
}

/// @brief      RC522 与卡片通信
///
/// @param      ucCmd       ：命令
///             pucSendData ：发送数据
///             ucSendLen   ：发送长度
///             pucBackData ：接收缓冲
///             pusBackBits ：接收 bit 数
///             emDevNum    ：设备号
///
/// @note
static uint8_t s_ucRc522ToCard(uint8_t ucCmd,
                               uint8_t *pucSendData,
                               uint8_t ucSendLen,
                               uint8_t *pucBackData,
                               uint16_t *pusBackBits,
                               emRc522DevNumTdf emDevNum)
{
    uint8_t ucStatus;
    uint8_t ucIrqEn;
    uint8_t ucWaitIrq;
    uint8_t ucLastBits;
    uint8_t ucN;
    uint16_t usI;

    ucStatus = RC522_STATUS_ERR;
    ucIrqEn = 0x00u;
    ucWaitIrq = 0x00u;

    if(ucCmd == RC522_CMD_AUTHENT)
    {
        ucIrqEn = 0x12u;
        ucWaitIrq = 0x10u;
    }
    else if(ucCmd == RC522_CMD_TRANSCEIVE)
    {
        ucIrqEn = 0x77u;
        ucWaitIrq = 0x30u;
    }
    else
    {
        return RC522_STATUS_ERR;
    }

    s_vRc522WriteReg(RC522_REG_COMMIEN, (uint8_t)(ucIrqEn | 0x80u), emDevNum);
    s_vRc522ClearBitMask(RC522_REG_COMMIRQ, 0x80u, emDevNum);
    s_vRc522SetBitMask(RC522_REG_FIFO_LEVEL, 0x80u, emDevNum);
    s_vRc522WriteReg(RC522_REG_COMMAND, RC522_CMD_IDLE, emDevNum);

    for(usI = 0u; usI < ucSendLen; usI++)
    {
        s_vRc522WriteReg(RC522_REG_FIFO_DATA, pucSendData[usI], emDevNum);
    }

    s_vRc522WriteReg(RC522_REG_COMMAND, ucCmd, emDevNum);
    if(ucCmd == RC522_CMD_TRANSCEIVE)
    {
        s_vRc522SetBitMask(RC522_REG_BIT_FRAMING, 0x80u, emDevNum);
    }

    usI = 2000u;
    do
    {
        ucN = s_ucRc522ReadReg(RC522_REG_COMMIRQ, emDevNum);
        usI--;
    } while((usI != 0u) && ((ucN & 0x01u) == 0u) && ((ucN & ucWaitIrq) == 0u));

    s_vRc522ClearBitMask(RC522_REG_BIT_FRAMING, 0x80u, emDevNum);

    if(usI != 0u)
    {
        if((s_ucRc522ReadReg(RC522_REG_ERROR, emDevNum) & 0x1Bu) == 0u)
        {
            ucStatus = RC522_STATUS_OK;

            if((ucN & 0x01u) != 0u)
            {
                ucStatus = RC522_STATUS_NO_TAG;
            }

            if(ucCmd == RC522_CMD_TRANSCEIVE)
            {
                ucN = s_ucRc522ReadReg(RC522_REG_FIFO_LEVEL, emDevNum);
                ucLastBits = (uint8_t)(s_ucRc522ReadReg(RC522_REG_CONTROL, emDevNum) & 0x07u);

                if(ucLastBits != 0u)
                {
                    *pusBackBits = (uint16_t)((ucN - 1u) * 8u + ucLastBits);
                }
                else
                {
                    *pusBackBits = (uint16_t)(ucN * 8u);
                }

                if(ucN == 0u)
                {
                    ucN = 1u;
                }

                if(ucN > 16u)
                {
                    ucN = 16u;
                }

                for(usI = 0u; usI < ucN; usI++)
                {
                    pucBackData[usI] = s_ucRc522ReadReg(RC522_REG_FIFO_DATA, emDevNum);
                }
            }
        }
    }

    return ucStatus;
}

/// @brief      寻卡
///
/// @param      pucTagType  ：卡片类型返回缓冲
///             emDevNum    ：设备号
///
/// @note
static uint8_t s_ucRc522Request(uint8_t *pucTagType, emRc522DevNumTdf emDevNum)
{
    uint8_t ucStatus;
    uint16_t usBackBits;

    s_vRc522WriteReg(RC522_REG_BIT_FRAMING, 0x07u, emDevNum);

    pucTagType[0] = RC522_PICC_REQIDL;
    ucStatus = s_ucRc522ToCard(RC522_CMD_TRANSCEIVE, pucTagType, 1u, pucTagType, &usBackBits, emDevNum);

    if((ucStatus != RC522_STATUS_OK) || (usBackBits != 0x10u))
    {
        ucStatus = RC522_STATUS_ERR;
    }

    return ucStatus;
}

/// @brief      防冲突，获取 UID
///
/// @param      pucUid      ：UID 输出缓冲（至少 4 字节）
///             emDevNum    ：设备号
///
/// @note
static uint8_t s_ucRc522AntiColl(uint8_t *pucUid, emRc522DevNumTdf emDevNum)
{
    uint8_t ucStatus;
    uint8_t ucI;
    uint8_t ucCheck;
    uint16_t usBackBits;
    uint8_t aucBuff[5];

    s_vRc522WriteReg(RC522_REG_BIT_FRAMING, 0x00u, emDevNum);

    aucBuff[0] = RC522_PICC_ANTICOLL;
    aucBuff[1] = 0x20u;

    ucStatus = s_ucRc522ToCard(RC522_CMD_TRANSCEIVE, aucBuff, 2u, aucBuff, &usBackBits, emDevNum);
    if(ucStatus == RC522_STATUS_OK)
    {
        ucCheck = 0u;
        for(ucI = 0u; ucI < 4u; ucI++)
        {
            pucUid[ucI] = aucBuff[ucI];
            ucCheck ^= aucBuff[ucI];
        }

        if(ucCheck != aucBuff[4])
        {
            ucStatus = RC522_STATUS_ERR;
        }
    }

    return ucStatus;
}

/// @brief      UID 转字符串
///
/// @param      c_pucUid    ：UID 缓冲
///             ucUidLen    ：UID 长度
///             pcOut       ：输出字符串
///             usMaxLen    ：输出缓冲大小
///
/// @note
void vRc522UidToString(const uint8_t *c_pucUid, uint8_t ucUidLen, char *pcOut, uint16_t usMaxLen)
{
    uint8_t ucI;
    uint16_t usPos;

    if((pcOut == NULL) || (usMaxLen == 0u))
    {
        return;
    }

    pcOut[0] = '\0';
    if((c_pucUid == NULL) || (ucUidLen == 0u))
    {
        return;
    }

    usPos = 0u;
    for(ucI = 0u; ucI < ucUidLen; ucI++)
    {
        if((uint16_t)(usPos + 2u) >= usMaxLen)
        {
            break;
        }

        s_vByteToHex(c_pucUid[ucI], &pcOut[usPos]);
        usPos += 2u;

        if((ucI != (uint8_t)(ucUidLen - 1u)) && ((uint16_t)(usPos + 1u) < usMaxLen))
        {
            pcOut[usPos] = ' ';
            usPos += 1u;
        }
    }

    if(usPos >= usMaxLen)
    {
        pcOut[usMaxLen - 1u] = '\0';
    }
    else
    {
        pcOut[usPos] = '\0';
    }
}

/// @brief      读卡 UID
///
/// @param      pucUid      ：UID 输出缓冲
///             pucUidLen   ：UID 长度输出
///             emDevNum    ：设备号
///
/// @note
uint8_t ucRc522ReadUid(uint8_t *pucUid, uint8_t *pucUidLen, emRc522DevNumTdf emDevNum)
{
    uint8_t ucStatus;
    uint8_t aucTagType[2];
    uint8_t aucUid[4];

    if((pucUid == NULL) || (pucUidLen == NULL))
    {
        return FALSE;
    }

    ucStatus = s_ucRc522Request(aucTagType, emDevNum);
    if(ucStatus != RC522_STATUS_OK)
    {
        return FALSE;
    }

    ucStatus = s_ucRc522AntiColl(aucUid, emDevNum);
    if(ucStatus != RC522_STATUS_OK)
    {
        return FALSE;
    }

    memcpy(pucUid, aucUid, sizeof(aucUid));
    *pucUidLen = 4u;

    memcpy(astRc522DeviceParam[emDevNum].stRunningParam.aucUid, aucUid, sizeof(aucUid));
    astRc522DeviceParam[emDevNum].stRunningParam.ucUidLen = 4u;

    return TRUE;
}

/// @brief      读卡 UID（十六进制字符串）
///
/// @param      pcUidHex    ：输出字符串缓冲
///             usMaxLen    ：输出缓冲大小
///             emDevNum    ：设备号
///
/// @note
uint8_t ucRc522ReadUidHex(char *pcUidHex, uint16_t usMaxLen, emRc522DevNumTdf emDevNum)
{
    uint8_t aucUid[10];
    uint8_t ucUidLen;

    if((pcUidHex == NULL) || (usMaxLen <= RC522_UID_HEX_MAX_LEN))
    {
        return FALSE;
    }

    if(ucRc522ReadUid(aucUid, &ucUidLen, emDevNum) == FALSE)
    {
        return FALSE;
    }

    vRc522UidToString(aucUid, ucUidLen, pcUidHex, usMaxLen);
    return TRUE;
}

/// @brief      UID 十六进制字符串转 32bit 键值
///
/// @param      pcUidHex    ：UID 字符串，如 "DD 9A F0 06"
///             pulUidKey   ：输出键值，如 0xDD9AF006
///
/// @note
uint8_t ucRc522UidHexToKey32(const char *pcUidHex, uint32_t *pulUidKey)
{
    uint8_t ucHigh;
    uint8_t ucLow;
    uint8_t ucIndex;
    uint32_t ulKey;

    if((pcUidHex == NULL) || (pulUidKey == NULL))
    {
        return FALSE;
    }

    ulKey = 0u;
    for(ucIndex = 0u; ucIndex < 4u; ucIndex++)
    {
        uint32_t ulPos;

        ulPos = (uint32_t)ucIndex * 3u;

        if(s_ucHexCharToNibble(pcUidHex[ulPos], &ucHigh) == FALSE)
        {
            return FALSE;
        }

        if(s_ucHexCharToNibble(pcUidHex[ulPos + 1u], &ucLow) == FALSE)
        {
            return FALSE;
        }

        ulKey = (ulKey << 8u) | (uint32_t)((uint8_t)((ucHigh << 4u) | ucLow));

        if((ucIndex < 3u) && (pcUidHex[ulPos + 2u] != ' '))
        {
            return FALSE;
        }
    }

    *pulUidKey = ulKey;
    return TRUE;
}

/// @brief      RC522 初始化
///
/// @param      pstInit     ：初始化参数结构体首地址
///             emDevNum    ：设备号
///
/// @note
void vRc522DeviceInit(stRc522StaticParamTdf *pstInit, emRc522DevNumTdf emDevNum)
{
    GPIO_InitTypeDef stGpioInit;

    if(pstInit == NULL)
    {
        return;
    }

    // 1. 初始化静态参数
    memcpy(&astRc522DeviceParam[emDevNum].stStaticParam,
           pstInit,
           sizeof(stRc522StaticParamTdf) / sizeof(uint8_t));

    // 2. 初始化运行参数
    memset(astRc522DeviceParam[emDevNum].stRunningParam.aucUid,
           0,
           sizeof(astRc522DeviceParam[emDevNum].stRunningParam.aucUid));
    astRc522DeviceParam[emDevNum].stRunningParam.ucUidLen = 0u;

    // 3. 初始化 NSS/RST 引脚
    stGpioInit.Mode = GPIO_MODE_OUTPUT_PP;
    stGpioInit.Pull = GPIO_NOPULL;
    stGpioInit.Speed = GPIO_SPEED_FREQ_HIGH;

    stGpioInit.Pin = astRc522DeviceParam[emDevNum].stStaticParam.usNssPin;
    HAL_GPIO_Init(astRc522DeviceParam[emDevNum].stStaticParam.pstNssPort, &stGpioInit);

    stGpioInit.Pin = astRc522DeviceParam[emDevNum].stStaticParam.usRstPin;
    HAL_GPIO_Init(astRc522DeviceParam[emDevNum].stStaticParam.pstRstPort, &stGpioInit);

    HAL_GPIO_WritePin(astRc522DeviceParam[emDevNum].stStaticParam.pstNssPort,
                      astRc522DeviceParam[emDevNum].stStaticParam.usNssPin,
                      GPIO_PIN_SET);

    HAL_GPIO_WritePin(astRc522DeviceParam[emDevNum].stStaticParam.pstRstPort,
                      astRc522DeviceParam[emDevNum].stStaticParam.usRstPin,
                      GPIO_PIN_SET);
    HAL_Delay(1u);
    HAL_GPIO_WritePin(astRc522DeviceParam[emDevNum].stStaticParam.pstRstPort,
                      astRc522DeviceParam[emDevNum].stStaticParam.usRstPin,
                      GPIO_PIN_RESET);
    HAL_Delay(1u);
    HAL_GPIO_WritePin(astRc522DeviceParam[emDevNum].stStaticParam.pstRstPort,
                      astRc522DeviceParam[emDevNum].stStaticParam.usRstPin,
                      GPIO_PIN_SET);
    HAL_Delay(2u);

    // 4. RC522 寄存器初始化
    s_vRc522Reset(emDevNum);
}