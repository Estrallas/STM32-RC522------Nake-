/**
  * @file       oled_device.c
  * @author     拉咯比哩
  * @version    V1.0.1
  * @date       20260326
  * @brief      OLED 驱动，基于 STM32 HAL 库
  *
  * <h2><center>&copy;此文件版权归【拉咯比哩】所有.</center></h2>
  */

#include "oled_device.h"
#include "string.h"
#include "math.h"
#include "oled_font.h"

stOledDeviceParamTdf astOledDeviceParam[OLED_DEV_NUM];

/// @brief      获取 OLED 设备参数
///
/// @param      emDevNum   ：设备号
///
/// @note       注意，返回值是 stOledDeviceParamTdf 型的指针，且指针指向的内容是不可更改的（只读的）
const stOledDeviceParamTdf *c_pstGetOledDeviceParam(emOledDevNumTdf emDevNum)
{
    return &astOledDeviceParam[emDevNum];
}

/// @brief      拷贝运行参数
///
/// @param      emDevNum   ：设备号
///
/// @note       
void vOledDeviceRunningParamInit(stOledRunningParamTdf *pstInit, emOledDevNumTdf emDevNum)
{
    memcpy(&astOledDeviceParam[emDevNum].stRunningParam, pstInit, sizeof(stOledRunningParamTdf) / sizeof(uint8_t));
}


/// @brief      IIC 写入 1 组数据
///
/// @param      ucCtrl      ：控制字节，0x00 命令，0x40 数据
///             pucData     ：数据首地址
///             usLength    ：数据长度
///             emDevNum    ：设备号
///
/// @note
static void s_vOledIicWriteBytes(uint8_t ucCtrl, uint8_t *pucData, uint16_t usLength, emOledDevNumTdf emDevNum)
{
    HAL_I2C_Mem_Write(astOledDeviceParam[emDevNum].stStaticParam.pstI2cHandle,
                      astOledDeviceParam[emDevNum].stStaticParam.usDevAddr,
                      ucCtrl,
                      I2C_MEMADD_SIZE_8BIT,
                      pucData,
                      usLength,
                      100u);
}

/// @brief      向 OLED 写一个字节命令
///
/// @param      ucCmd       ：要写入的命令
///             emDevNum    ：设备号
///
/// @note
static void s_vOledWriteOneByteCmd(uint8_t ucCmd, emOledDevNumTdf emDevNum)
{
    s_vOledIicWriteBytes(0x00, &ucCmd, 1u, emDevNum);
}


/// @brief      向 OLED 写多个字节数据
///
/// @param      ucCmd       ：要写入的命令
///             ulLength    ：数据长度
///             emDevNum    ：设备号
///
/// @note
static void s_vOledWriteFewBytesData(uint8_t *pucData, uint32_t ulLength, emOledDevNumTdf emDevNum)
{
    s_vOledIicWriteBytes(0x40, pucData, (uint16_t)ulLength, emDevNum);
}

/// @brief      向缓冲区指定位置画 1 个点
///
/// @param      x           ：X 坐标，取值范围 0 ~ OLED_BUFFER_WIDTH - 1
///             y           ：Y 坐标，取值范围 0 ~ OLED_BUFFER_HEIGHT - 1
///             emDevNum    ：设备号
///
/// @note
void vOledDrawOnePointToBuffer(uint32_t x, uint32_t y, emOledDevNumTdf emDevNum)
{
    uint16_t ii, jj, kk;                        // x = 13, y = 20
    
    // 计算点属于哪个 Page
    ii = y >> 3;             // ii = y / 8      // ii = 2
    
    // 计算点属于 Seg 的哪一位
    jj = y & 0x07;           // jj = y % 8      // jj = 4
    
    // 计算对应的 Seg 掩码                      // kk = 0x01 << 4 = 0x10
    kk = 0x01 << jj;
    
    // 对应位置为 1
    astOledDeviceParam[emDevNum].stRunningParam.aucOledBuffer[ii * OLED_BUFFER_WIDTH + x] |= kk;
}

/// @brief      向缓冲区指定位置清除 1 个点
///
/// @param      x           ：X 坐标，取值范围 0 ~ OLED_BUFFER_WIDTH - 1
///             y           ：Y 坐标，取值范围 0 ~ OLED_BUFFER_HEIGHT - 1
///             emDevNum    ：设备号
///
/// @note
void vOledClearOnePointToBuffer(uint32_t x, uint32_t y, emOledDevNumTdf emDevNum)
{
    uint16_t ii, jj, kk;                        // x = 13, y = 20
    
    // 计算点属于哪个 Page
    ii = y >> 3;             // ii = y / 8      // ii = 2
    
    // 计算点属于 Seg 的哪一位
    jj = y & 0x07;           // jj = y % 8      // jj = 4
    
    // 计算对应的 Seg 掩码                      // kk = 0x01 << 4 = 0x10
    kk = 0x01 << jj;
    
    // 对应位置为 1
    astOledDeviceParam[emDevNum].stRunningParam.aucOledBuffer[ii * OLED_BUFFER_WIDTH + x] &= ~kk;
}

/// @brief      向缓存写入 1 个字符
///
/// @param      x           ：X 坐标，取值范围 0 ~ OLED_POINT_WIDTH - 1
///             y           ：Y 坐标，取值范围 0 ~ OLED_POINT_HEIGHT - 1
///             ucChar      ：要显示的字符
///             emFontSize  ：字号，见 emOledFontSizeTdf 定义
///             emMode      ：显示模式，见 emOledPixelShowModeTdf 定义
///             emDevNum    ：设备号
///
/// @note
void vOledWriteOneCharToBuffer(uint32_t x, uint32_t y, uint8_t ucChar, emOledFontSizeTdf emFontSize, emOledPixelShowModeTdf emMode, emOledDevNumTdf emDevNum)
{
    uint16_t i, j;
    uint16_t y0;
    uint32_t ulSizeTemp, ulCharIndex;
    uint8_t ucData;
    const uint8_t *p;
    
    // 1. 计算显示 1 个字符，需要写多少 BYTE
    //ucSizeTemp = ((emFontSize / 8) + ((emFontSize % 8) ? 1 : 0)) * (emFontSize / 2);        // 除法和求模太慢了，用位移和逻辑运算代替
    ulSizeTemp = ((emFontSize >> 3) + ((emFontSize & 0x07) ? 1 : 0)) * (emFontSize >> 1);
    
    // 2. 计算要显示字符，在字模数组中的索引
    ulCharIndex = ucChar - ' ';
    
    // 3. 初始化 y0
    y0 = y;
    
    // 4. 将字符写入缓存
    // 4.1. 选择字模数组
    if(emFontSize == emOledFontSize_6x12)
    {
        p = (const uint8_t *)c_a2ucOledFont_Ascii_1206;
    }
    else if(emFontSize == emOledFontSize_8x16)
    {
        p = (const uint8_t *)c_a2ucOledFont_Ascii_1608;
    }
    else if(emFontSize == emOledFontSize_12x24)
    {
        p = (const uint8_t *)c_a2ucOledFont_Ascii_2412;
    }
    else 
    {
        return;
    }
        
    // 4.2. 按字模将数据写入显存
    for(i = 0; i < ulSizeTemp; i++)
    {
        // 4.2.1. 更新要写的字符
        ucData = p[ulCharIndex * ulSizeTemp + i];
        
        // 4.2.2. 写入缓存
        for(j = 0; j < 8; j++)
        {

            if((ucData & 0x80) != 0)
            {
                if(emMode == emOledPixelShowMode_Positive)
                {
                    //vOledDrawOnePointToBuffer(x, y, emDevNum);
                    uint16_t ii, jj, kk;                        // x = 13, y = 20
    
                    // 计算点属于哪个 Page
                    ii = y >> 3;             // ii = y / 8      // ii = 2
                    
                    // 计算点属于 Seg 的哪一位
                    jj = y & 0x07;           // jj = y % 8      // jj = 4
                    
                    // 计算对应的 Seg 掩码                      // kk = 0x01 << 4 = 0x10
                    kk = 0x01 << jj;
                    
                    // 对应位置为 1
                    astOledDeviceParam[emDevNum].stRunningParam.aucOledBuffer[ii * OLED_BUFFER_WIDTH + x] |= kk;
                }
                else
                {
                    //vOledClearOnePointToBuffer(x, y, emDevNum);
                    uint16_t ii, jj, kk;                        // x = 13, y = 20
    
                    // 计算点属于哪个 Page
                    ii = y >> 3;             // ii = y / 8      // ii = 2
                    
                    // 计算点属于 Seg 的哪一位
                    jj = y & 0x07;           // jj = y % 8      // jj = 4
                    
                    // 计算对应的 Seg 掩码                      // kk = 0x01 << 4 = 0x10
                    kk = 0x01 << jj;
                    
                    // 对应位置为 1
                    astOledDeviceParam[emDevNum].stRunningParam.aucOledBuffer[ii * OLED_BUFFER_WIDTH + x] &= ~kk;
                }
            }
            else
            {
                if(emMode == emOledPixelShowMode_Positive)
                {
                    //vOledClearOnePointToBuffer(x, y, emDevNum);
                    uint16_t ii, jj, kk;                        // x = 13, y = 20
    
                    // 计算点属于哪个 Page
                    ii = y >> 3;             // ii = y / 8      // ii = 2
                    
                    // 计算点属于 Seg 的哪一位
                    jj = y & 0x07;           // jj = y % 8      // jj = 4
                    
                    // 计算对应的 Seg 掩码                      // kk = 0x01 << 4 = 0x10
                    kk = 0x01 << jj;
                    
                    // 对应位置为 1
                    astOledDeviceParam[emDevNum].stRunningParam.aucOledBuffer[ii * OLED_BUFFER_WIDTH + x] &= ~kk;
                }
                else
                {
                    //vOledDrawOnePointToBuffer(x, y, emDevNum);
                    uint16_t ii, jj, kk;                        // x = 13, y = 20
                    
                    // 计算点属于哪个 Page
                    ii = y >> 3;             // ii = y / 8      // ii = 2
                    
                    // 计算点属于 Seg 的哪一位
                    jj = y & 0x07;           // jj = y % 8      // jj = 4
                    
                    // 计算对应的 Seg 掩码                      // kk = 0x01 << 4 = 0x10
                    kk = 0x01 << jj;
                    
                    // 对应位置为 1
                    astOledDeviceParam[emDevNum].stRunningParam.aucOledBuffer[ii * OLED_BUFFER_WIDTH + x] |= kk;
                }
            }
            
            ucData <<= 1;
            
            y++;
            if((y - y0) == (uint16_t)emFontSize)            // 当前列显示完成，显示下一列
            {
                y = y0;
                x++;
                break;
            }
        }
    }
}


/// @brief      向缓存写入字符串
///
/// @param      x           ：X 坐标，取值范围 0 ~ OLED_POINT_WIDTH - 1
///             y           ：Y 坐标，取值范围 0 ~ OLED_POINT_HEIGHT - 1
///             pucChar     ：要显示的字符串首地址指针
///             emFontSize  ：字号，见 emOledFontSizeTdf 定义
///             emMode      : 显示模式，见 emOledPixelShowModeTdf 定义
///             emDevNum    ：设备号
///
/// @note
void vUiWriteStringToBuffer(uint16_t x, uint16_t y, const uint8_t *c_pucChar, emOledFontSizeTdf emFontSize, emOledPixelShowModeTdf emMode, emOledDevNumTdf emDevNum)
{
    while((*c_pucChar >= ' ') && (*c_pucChar <= '~'))           // 判断字符是否合法
    {
        vOledWriteOneCharToBuffer(x, y, *c_pucChar, emFontSize, emMode, emDevNum);
        
        x += emFontSize >> 1;
        
        if(x > (OLED_POINT_WIDTH - emFontSize))              // 换行
        {
            x  = 0;
            y += emFontSize;
        }
        
        c_pucChar++;
    }
}

/// @brief      更新显存显示
///
/// @param      emDevNum        ：设备号
///
/// @note
void vOledRefreshFromBuffer(emOledDevNumTdf emDevNum)
{
    uint8_t i;
    
    
    
    for(i = 0; i < OLED_BUFFER_HEIGHT; i++)
    {
        s_vOledWriteOneByteCmd(0xB0 + i, emDevNum);     // 设置行起始地址
        s_vOledWriteOneByteCmd(0x00, emDevNum);         // 设置低列起始地址
        s_vOledWriteOneByteCmd(0x10, emDevNum);         // 设置高列起始地址
        
        s_vOledWriteFewBytesData(&(astOledDeviceParam[emDevNum].stRunningParam.aucOledBuffer[i * OLED_BUFFER_WIDTH]), 
                                 OLED_BUFFER_WIDTH, 
                                 emDevNum);
    }
}

/// @brief      OLED 命令初始化
///
/// @param      emDevNum    ：设备号
///
/// @note
void vOledCmdInit(emOledDevNumTdf emDevNum)
{
	s_vOledWriteOneByteCmd(0xAE, emDevNum);//--turn off oled panel
	s_vOledWriteOneByteCmd(0x00, emDevNum);//---set low column address
	s_vOledWriteOneByteCmd(0x10, emDevNum);//---set high column address
	s_vOledWriteOneByteCmd(0x40, emDevNum);//--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
	s_vOledWriteOneByteCmd(0x81, emDevNum);//--set contrast control register
	s_vOledWriteOneByteCmd(0xCF, emDevNum);// Set SEG Output Current Brightness
	s_vOledWriteOneByteCmd(0xA1, emDevNum);//--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
	s_vOledWriteOneByteCmd(0xC8, emDevNum);//Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
	s_vOledWriteOneByteCmd(0xA6, emDevNum);//--set normal display
	s_vOledWriteOneByteCmd(0xA8, emDevNum);//--set multiplex ratio(1 to 64)
	s_vOledWriteOneByteCmd(0x3f, emDevNum);//--1/64 duty
	s_vOledWriteOneByteCmd(0xD3, emDevNum);//-set display offset	Shift Mapping RAM Counter (0x00~0x3F)
	s_vOledWriteOneByteCmd(0x00, emDevNum);//-not offset
	s_vOledWriteOneByteCmd(0xd5, emDevNum);//--set display clock divide ratio/oscillator frequency
	s_vOledWriteOneByteCmd(0x80, emDevNum);//--set divide ratio, Set Clock as 100 Frames/Sec
	s_vOledWriteOneByteCmd(0xD9, emDevNum);//--set pre-charge period
	s_vOledWriteOneByteCmd(0xF1, emDevNum);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	s_vOledWriteOneByteCmd(0xDA, emDevNum);//--set com pins hardware configuration
	s_vOledWriteOneByteCmd(0x12, emDevNum);
	s_vOledWriteOneByteCmd(0xDB, emDevNum);//--set vcomh
	s_vOledWriteOneByteCmd(0x40, emDevNum);//Set VCOM Deselect Level
	s_vOledWriteOneByteCmd(0x20, emDevNum);//-Set Page Addressing Mode (0x00/0x01/0x02)
	s_vOledWriteOneByteCmd(0x02, emDevNum);//
	s_vOledWriteOneByteCmd(0x8D, emDevNum);//--set Charge Pump enable/disable
	s_vOledWriteOneByteCmd(0x14, emDevNum);//--set(0x10) disable
	s_vOledWriteOneByteCmd(0xA4, emDevNum);// Disable Entire Display On (0xa4/0xa5)
	s_vOledWriteOneByteCmd(0xA6, emDevNum);// Disable Inverse Display On (0xa6/a7) 
	s_vOledWriteOneByteCmd(0xAF, emDevNum);
}



/// @brief      OLED 设备初始化
///
/// @param      pstInit     ：初始化参数结构体的首地址
/// @param      emDevNum    ：设备编号
///
/// @note
void vOledDeviceInit(stOledStaticParamTdf *pstInit, emOledDevNumTdf emDevNum)
{
    // 1. 初始化静态参数
    memcpy(&astOledDeviceParam[emDevNum].stStaticParam, pstInit, sizeof(stOledStaticParamTdf) / sizeof(uint8_t));

    // 2. 初始化寄存器
    vOledCmdInit(emDevNum);
}

