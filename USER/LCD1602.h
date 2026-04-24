#ifndef __LCD1602_H
#define __LCD1602_H 
#include "sys.h"
#include "stdlib.h"	
 

 
//----------------OLED????----------------- 
/***************????????****************/
#define LCD1602_RS_GPIO_PORT				GPIOC
#define LCD1602_RS_GPIO_PIN					GPIO_Pin_13
				
#define LCD1602_RW_GPIO_PORT				GPIOC
#define LCD1602_RW_GPIO_PIN					GPIO_Pin_14
				
#define LCD1602_E_GPIO_PORT					GPIOC
#define LCD1602_E_GPIO_PIN					GPIO_Pin_15
				
 
/*********************END**********************/
 
 
#define LCD1602_RS_H   	GPIO_SetBits(LCD1602_RS_GPIO_PORT,LCD1602_RS_GPIO_PIN)
#define LCD1602_RS_L	 	GPIO_ResetBits(LCD1602_RS_GPIO_PORT,LCD1602_RS_GPIO_PIN)
 
#define LCD1602_RW_H		GPIO_SetBits(LCD1602_RW_GPIO_PORT,LCD1602_RW_GPIO_PIN)
#define LCD1602_RW_L		GPIO_ResetBits(LCD1602_RW_GPIO_PORT,LCD1602_RW_GPIO_PIN)
 
#define LCD1602_E_H   	GPIO_SetBits(LCD1602_E_GPIO_PORT,LCD1602_E_GPIO_PIN)
#define LCD1602_E_L  		GPIO_ResetBits(LCD1602_E_GPIO_PORT, LCD1602_E_GPIO_PIN)
 
 
 
void LCD1602_Init(void);
void LCD1602_WriteCom(uint8_t com);
void LCD1602_WriteData(uint8_t dat);
void LCD1602_Write_String(unsigned char x,unsigned char y,unsigned char *s);
void LCD1602_Write_Char(unsigned char x,unsigned char y,unsigned char Data); 
void LCD1602_Clear(void); 
 
 
#endif

