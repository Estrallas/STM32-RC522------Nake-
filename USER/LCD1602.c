#include "lcd1602.h"
#include "stdlib.h" 	 
#include "delay.h"


void LCD1602_Init(void)
{		
	/*????GPIO_InitTypeDef??????*/
	GPIO_InitTypeDef GPIO_InitStructure;
 
	/*??LED???GPIO????*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOB,ENABLE);
//LCD_RS?????
	GPIO_InitStructure.GPIO_Pin = LCD1602_RS_GPIO_PIN;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
	GPIO_Init(GPIOC, &GPIO_InitStructure);	
	
//LCD_RW?????
	GPIO_InitStructure.GPIO_Pin = LCD1602_RW_GPIO_PIN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
//LCD_E?????
	GPIO_InitStructure.GPIO_Pin = LCD1602_E_GPIO_PIN;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
 
//D0-D7?????
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15|GPIO_Pin_12|GPIO_Pin_11|GPIO_Pin_10|GPIO_Pin_9|GPIO_Pin_8;
	GPIO_Init(GPIOA, &GPIO_InitStructure); 
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
	GPIO_Init(GPIOB, &GPIO_InitStructure); 
	
	LCD1602_WriteCom(0x38);  //开显示
	LCD1602_WriteCom(0x0c);  //开显示不显示光标
	LCD1602_WriteCom(0x06);  //写一个指针加1
	LCD1602_WriteCom(0x01);  //清屏
	LCD1602_WriteCom(0x80);  //设置数据指针起点
 
}
 
void LCD1602_DATAPINS(uint8_t Date)
{
	uint16_t dat_buf;
	dat_buf=GPIO_ReadOutputData(GPIOA)&&GPIO_ReadOutputData(GPIOB);
	dat_buf=dat_buf&0xFF00;
	dat_buf=dat_buf|Date;
 
	GPIO_Write(GPIOA, dat_buf);
	GPIO_Write(GPIOB, dat_buf);
	
}
 
 
 
void LCD1602_WriteCom(uint8_t com)	  //????
{
	LCD1602_RS_L;	   
	LCD1602_RW_L;	  
	LCD1602_E_L;     
 
	LCD1602_DATAPINS(com);     //????
	delay_ms(1);		//???  ???
 
	LCD1602_E_H;	          //????
	delay_ms(5);		  //????
	LCD1602_E_L;
 
}
 
/*******************************************************************************
* ? ? ?         : LcdWriteData
* ????		   : ?LCD?????????
* ?    ?         : dat
* ?    ?         : ?
*******************************************************************************/		   
   
void LCD1602_WriteData(uint8_t dat)			//????
{
	LCD1602_RS_H;	   //??????
	LCD1602_RW_L;	   //????
	LCD1602_E_L;     //??
	
	LCD1602_DATAPINS(dat);  //????
	delay_ms(1);
 
	LCD1602_E_H;   //????
	delay_ms(5);   //????
	LCD1602_E_L;
 
}
 
/*------------------------------------------------
              ??????
------------------------------------------------*/
 void LCD1602_Write_Char(unsigned char x,unsigned char y,unsigned char Data) 
{     
 if (y == 0) 
 	{     
 	LCD1602_WriteCom(0x80 + x);    //???    
 	}    
 else 
 	{     
 	LCD1602_WriteCom(0xC0 + x);  //???     
 	}        
 LCD1602_WriteData( Data); //????      
}
 
 
 
 
/*------------------------------------------------
              ???????
------------------------------------------------*/
 void LCD1602_Write_String(unsigned char x,unsigned char y,unsigned char *s) 
 {     
 if (y == 0) 
 	{     
	 LCD1602_WriteCom(0x80 + x);  //???   
 	}
 else 
 	{     
 	LCD1602_WriteCom(0xC0 + x);  //???   
 	}        
	while (*s) //??????????
 	{     
		 LCD1602_WriteData( *s);//????     
		 s ++;  //???1   
 	}
 }
 
  /*------------------------------------------------
                ????
------------------------------------------------*/
 void LCD1602_Clear(void) 
{ 
 LCD1602_WriteCom(0x01); 
 delay_ms(5);
}


