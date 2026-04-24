#ifndef __RC522CONF_H__
#define __RC522CONF_H__

#include "stm32f10x.h"

/**************���Ŷ���**************/
// ����RCCʱ�� (GPIOA + GPIOB 用于 RC522)
#define RC522_GPIO_RCCLOCK      (RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB)

// ��λ���� (RST -> PA8)
#define RST_PORT     GPIOA
#define RST_PIN      GPIO_Pin_8
// ����ӳ����ţ����ݽ������� (MISO -> PB14)
#define MISO_PORT    GPIOB
#define MISO_PIN     GPIO_Pin_14
// �����������ţ����ݷ������� (MOSI -> PB15)
#define MOSI_PORT    GPIOB
#define MOSI_PIN     GPIO_Pin_15
// ʱ������ (SCK -> PB13)
#define SCK_PORT     GPIOB
#define SCK_PIN      GPIO_Pin_13
// Ƭѡ���� (SDA/SS -> PB12)
#define SDA_PORT     GPIOB
#define SDA_PIN      GPIO_Pin_12


// ���ŵ�ƽ����
#define RSTHIGH GPIO_SetBits(RST_PORT, RST_PIN)
#define RSTLOW GPIO_ResetBits(RST_PORT, RST_PIN)
#define MOSIHIGH GPIO_SetBits(MOSI_PORT, MOSI_PIN)
#define MOSILOW GPIO_ResetBits(MOSI_PORT, MOSI_PIN)
#define SCKHIGH GPIO_SetBits(SCK_PORT, SCK_PIN)
#define SCKLOW GPIO_ResetBits(SCK_PORT, SCK_PIN)
#define SDAHIGH GPIO_SetBits(SDA_PORT, SDA_PIN)
#define SDALOW GPIO_ResetBits(SDA_PORT, SDA_PIN)
#define MISOVALUE GPIO_ReadInputDataBit(MISO_PORT, MISO_PIN)

/* MF522 �����֣�����RC522ģ��Ĵ��������� ------------------------------------------------- */
#define PCD_IDLE              0x00               //ȡ����ǰ����
#define PCD_AUTHENT           0x0E               //��֤��Կ
#define PCD_RECEIVE           0x08               //��������
#define PCD_TRANSMIT          0x04               //��������
#define PCD_TRANSCEIVE        0x0C               //���Ͳ���������
#define PCD_RESETPHASE        0x0F               //��λ
#define PCD_CALCCRC           0x03               //CRC����

/* ifare_One��Ƭ�����֣�ͨ��RC522ģ�����MIFARE��Ƭ������ ---------------------------------------------------- */
#define PICC_REQIDL           0x26               //Ѱ��������δ��������״̬
#define PICC_REQALL           0x52               //Ѱ��������ȫ����
#define PICC_ANTICOLL1        0x93               //����ײ
#define PICC_ANTICOLL2        0x95               //����ײ
#define PICC_AUTHENT1A        0x60               //��֤A��Կ
#define PICC_AUTHENT1B        0x61               //��֤B��Կ
#define PICC_READ             0x30               //����
#define PICC_WRITE            0xA0               //д��
#define PICC_DECREMENT        0xC0               //�ۿ�
#define PICC_INCREMENT        0xC1               //��ֵ
#define PICC_RESTORE          0xC2               //�������ݵ�������
#define PICC_TRANSFER         0xB0               //���滺����������
#define PICC_HALT             0x50               //����

/* MF522 FIFO--------------------------- */
#define DEF_FIFO_LENGTH       64
#define MAXRLEN               18

/* MF522�Ĵ��� ------------------------- */
// PAGE 0
#define     RFU00                 0x00    
#define     CommandReg            0x01    
#define     ComIEnReg             0x02    
#define     DivlEnReg             0x03    
#define     ComIrqReg             0x04    
#define     DivIrqReg             0x05
#define     ErrorReg              0x06    
#define     Status1Reg            0x07    
#define     Status2Reg            0x08    
#define     FIFODataReg           0x09
#define     FIFOLevelReg          0x0A
#define     WaterLevelReg         0x0B
#define     ControlReg            0x0C
#define     BitFramingReg         0x0D
#define     CollReg               0x0E
#define     RFU0F                 0x0F
// PAGE 1     
#define     RFU10                 0x10
#define     ModeReg               0x11
#define     TxModeReg             0x12
#define     RxModeReg             0x13
#define     TxControlReg          0x14
#define     TxAutoReg             0x15
#define     TxSelReg              0x16
#define     RxSelReg              0x17
#define     RxThresholdReg        0x18
#define     DemodReg              0x19
#define     RFU1A                 0x1A
#define     RFU1B                 0x1B
#define     MifareReg             0x1C
#define     RFU1D                 0x1D
#define     RFU1E                 0x1E
#define     SerialSpeedReg        0x1F
// PAGE 2    
#define     RFU20                 0x20  
#define     CRCResultRegM         0x21
#define     CRCResultRegL         0x22
#define     RFU23                 0x23
#define     ModWidthReg           0x24
#define     RFU25                 0x25
#define     RFCfgReg              0x26
#define     GsNReg                0x27
#define     CWGsCfgReg            0x28
#define     ModGsCfgReg           0x29
#define     TModeReg              0x2A
#define     TPrescalerReg         0x2B
#define     TReloadRegH           0x2C
#define     TReloadRegL           0x2D
#define     TCounterValueRegH     0x2E
#define     TCounterValueRegL     0x2F
// PAGE 3      
#define     RFU30                 0x30
#define     TestSel1Reg           0x31
#define     TestSel2Reg           0x32
#define     TestPinEnReg          0x33
#define     TestPinValueReg       0x34
#define     TestBusReg            0x35
#define     AutoTestReg           0x36
#define     VersionReg            0x37
#define     AnalogTestReg         0x38
#define     TestDAC1Reg           0x39  
#define     TestDAC2Reg           0x3A   
#define     TestADCReg            0x3B   
#define     RFU3C                 0x3C   
#define     RFU3D                 0x3D   
#define     RFU3E                 0x3E   
#define     RFU3F		  		  0x3F

/* ͨ�ŷ����� ----------------------- */
#define 	MI_OK                 0
#define 	MI_NOTAGERR           1
#define 	MI_ERR                2

#endif

