#ifndef __OLED_H
#define __OLED_H			  	 
#include "sys.h"
#include "stdlib.h"	    	
//OLEDģʽ����
//0:4�ߴ���ģʽ
//1:����8080ģʽ
#define SIZE            16
#define XLevelL         0x00
#define XLevelH         0x10
#define Max_Column      128
#define Max_Row         64
#define Brightness      0xFF
#define X_WIDTH         128
#define Y_WIDTH         64
#define OLED_IIC        1
#define OLED_SPI        0
#define OLED_MODE       OLED_IIC
//-------------------GPIO���Ŷ���
#if (OLED_MODE==OLED_IIC)
//��ʼ���ӿڶ���
#define OLED_SCL_Pin          GPIO_Pin_8
#define OLED_SCL_GPIO_Port    GPIOB
#define OLED_SDA_Pin      	  GPIO_Pin_9
#define OLED_SDA_GPIO_Port    GPIOB
#define OLED_I2C               I2C1
#define OLED_I2C_REMAP         ENABLE
#define OLED_I2C_SPEED         100000
#define OLED_I2C_ADDR          0x78


//�ӿڲ�������
#define OLED_SCLK_Clr()     GPIO_ResetBits(OLED_SCL_GPIO_Port,OLED_SCL_Pin)//CLK
#define OLED_SCLK_Set()     GPIO_SetBits(OLED_SCL_GPIO_Port,OLED_SCL_Pin)
#define OLED_SDIN_Clr()     GPIO_ResetBits(OLED_SDA_GPIO_Port,OLED_SDA_Pin)//DIN
#define OLED_SDIN_Set()     GPIO_SetBits(OLED_SDA_GPIO_Port,OLED_SDA_Pin)
#define OLED_RST_Clr()      GPIO_ResetBits(OLED_RES_GPIO_Port,OLED_RES_Pin)//RES
#define OLED_RST_Set()      GPIO_SetBits(OLED_RES_GPIO_Port,OLED_RES_Pin)
#define OLED_DC_Clr()       GPIO_ResetBits(OLED_DC_GPIO_Port,OLED_DC_Pin)//DC
#define OLED_DC_Set()       GPIO_SetBits(OLED_DC_GPIO_Port,OLED_DC_Pin)
#define OLED_CS_Clr()       GPIO_ResetBits(OLED_CS_GPIO_Port,OLED_CS_Pin)//CS
#define OLED_CS_Set()       GPIO_SetBits(OLED_CS_GPIO_Port,OLED_CS_Pin)
#endif
#define OLED_CMD  0 //д����
#define OLED_DATA 1 //д����
#define PHOTO_SIZE 16*64
extern const unsigned char pic[];

// ��������
void OLED_WR_Byte(u8 dat,u8 cmd);
void OLED_Display_On(void);
void OLED_Display_Off(void);
void OLED_Init(void);
void OLED_Clear(u8 mode);
void OLED12864_ShowPhoto(u8 *p);
void OLED_DrawPoint(u8 x,u8 y,u8 t);
void OLED_Fill(u8 x1,u8 y1,u8 x2,u8 y2,u8 dot);
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 flag);
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len);
void OLED_ShowString(u8 x,u8 y, u8 *p, u8 size);
void OLED_Set_Pos(unsigned char x, unsigned char y);
void OLED_ShowCHineseWord(u8 x,u8 y,u8* str,u8 flag);
void OLED_ShowText(u8 x,u8 y,u8* str,u8 flag);
void OLED_ShowChinese(u8 x,u8 y,u8 no);
void OLED_DrawBMP(unsigned char x0, unsigned char y0,unsigned char x1, unsigned char y1,const unsigned char BMP[]);

#endif // __OLED_H
