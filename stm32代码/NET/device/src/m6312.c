/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	m6312.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2017-10-20
	*
	*	版本： 		V1.0
	*
	*	说明： 		M6312驱动
	*
	*	修改记录：	
	************************************************************
	************************************************************
	************************************************************
**/

//单片机头文件
#include "stm32f10x.h"

//网络设备驱动
#include "m6312.h"
#include "onenet.h"
//硬件驱动
#include "delay.h"
#include "usart.h"
#include "lcd.h"

//C库
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#define M6312_ONENET_INFO		"AT+CIPSTART=\"TCP\",\"mqtt.heclouds.com\",6002\r\n"

extern unsigned char *dataPtr;
unsigned char m6312_buf[128];
unsigned short m6312_cnt = 0, m6312_cntPre = 0;
u8 SIM900_CSQ[3];
u8 dtbuf[50];   								//打印缓存器	
u8 Flag_Rec_Call=0;

//==========================================================
//	函数名称：	M6312_Clear
//
//	函数功能：	清空缓存
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void M6312_Clear(void)
{

	memset(m6312_buf, 0, sizeof(m6312_buf));
	m6312_cnt = 0;

}

//==========================================================
//	函数名称：	M6312_WaitRecive
//
//	函数功能：	等待接收完成
//
//	入口参数：	无
//
//	返回参数：	REV_OK-接收完成		REV_WAIT-接收超时未完成
//
//	说明：		循环调用检测是否接收完成
//==========================================================
_Bool M6312_WaitRecive(void)
{

	if(m6312_cnt == 0) 							//如果接收计数为0 则说明没有处于接收数据中，所以直接跳出，结束函数
		return REV_WAIT;
		
	if(m6312_cnt == m6312_cntPre)				//如果上一次的值和这次相同，则说明接收完毕
	{
		m6312_cnt = 0;							//清0接收计数
			
		return REV_OK;							//返回接收完成标志
	}
		
	m6312_cntPre = m6312_cnt;					//置为相同
	
	return REV_WAIT;							//返回接收未完成标志

}

//==========================================================
//	函数名称：	M6312_SendCmd
//
//	函数功能：	发送命令
//
//	入口参数：	cmd：命令
//				res：需要检查的返回指令
//
//	返回参数：	0-成功	1-失败
//
//	说明：		
//==========================================================
_Bool M6312_SendCmd(char *cmd, char *res)
{
	
	unsigned char timeOut = 200;

	Usart_SendString(USART2, (unsigned char *)cmd, strlen((const char *)cmd));
	
	while(timeOut--)
	{
		if(M6312_WaitRecive() == REV_OK)							//如果收到数据
		{
			if(strstr((const char *)m6312_buf, res) != NULL)		//如果检索到关键词
			{
				M6312_Clear();										//清空缓存
				
				return 0;
			}
		}
		
		delay_ms(10);
	}
	
	return 1;

}

//==========================================================
//	函数名称：	M6312_SendData
//
//	函数功能：	发送数据
//
//	入口参数：	data：数据
//				len：长度
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void M6312_SendData(unsigned char *data, unsigned short len)
{

	char cmdBuf[32];
	
	M6312_Clear();		//清空接收缓存
	sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);	//发送命令
	if(!M6312_SendCmd(cmdBuf, ">"))				//收到‘>’时可以发送数据
	{
		Usart_SendString(USART2, data, len);	//发送设备连接请求数据
	}

}

//==========================================================
//	函数名称：	M6312_GetIPD
//
//	函数功能：	获取平台返回的数据
//
//	入口参数：	timeOut等待的时间(乘以10ms)
//
//	返回参数：	平台返回的原始数据
//
//	说明：		不同网络设备返回的格式不同，需要去调试
//==========================================================
unsigned char *M6312_GetIPD(unsigned short timeOut)
{

	char *ptrIPD;
	
	do
	{
		if(M6312_WaitRecive() == REV_OK)								//如果接收完成
		{
			ptrIPD = strstr((char *)m6312_buf, "IPD,");				//搜索“IPDATA”头
			if(ptrIPD == NULL)											//如果没找到，可能是IPDATA头的延迟，还是需要等待一会，但不会超过设定的时间
			{
			}
			else
			{
				ptrIPD = strchr(ptrIPD, ':');							//找到'\n'
				if(ptrIPD != NULL)
				{
					ptrIPD++;
					return (unsigned char *)(ptrIPD);
				}
				else
					return NULL;
			}
		}
		
		delay_ms(10);													//延时等待
	} while(timeOut--);
	
	return NULL;														//超时还未找到，返回空指针

}

//==========================================================
//	函数名称：	M6312_Init
//
//	函数功能：	初始化M6312
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void M6312_Init(void)
{
	
	M6312_Clear();
  UsartPrintf(USART2,"AT+CIPCLOSE\r\n");  //先断开服务器
  LCD_ShowChinese(43,  128,"检测模块中",RED,WHITE,16,0);
	while(M6312_SendCmd("AT\r\n", "OK"))
		delay_ms(500);
	
  LCD_ShowChinese(43,  128,"检测手机卡",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CPIN?\r\n", "+CPIN: READY"))		//确保SIM卡PIN码解锁，返回READY，表示解锁成功
		delay_ms(500);
	
  LCD_ShowChinese(43,  128,"检测信号值",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CREG?\r\n", "0,1"))  //0,1
		delay_ms(500);
	
  LCD_ShowChinese(43,  128,"注册网络中",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CGREG?\r\n","OK"))					//检查网络注册状态
		delay_ms(500);
	
  LCD_ShowString(43,  128,"AT+CIPSHUT",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CIPSHUT\r\n","SHUT OK"))				//关闭移动场景 
		delay_ms(500);

  LCD_ShowString(43,  128,"AT+CGCLASS",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CGCLASS=\"B\"\r\n","OK"))				//设置GPRS移动台类别为B,支持包交换和数据交换
		delay_ms(500);
	
  LCD_ShowString(43,  128,"AT+CGDCONT",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CGDCONT=1,\"IP\",\"CMNET\"\r\n","OK"))				//设置PDP上下文,互联网接协议,接入点等信息
		delay_ms(500);	
	
  LCD_ShowString(43,  128,"AT+CGATT",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CGATT=1\r\n","OK"))					//附着GPRS业务
		delay_ms(500);
	
  LCD_ShowString(43,  128,"AT+CIPMUX",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CIPMUX=0\r\n","OK"))					//必须为单连接，不然平台IP都连不上
		delay_ms(500);
	
  LCD_ShowString(43,  128,"AT+CIPHEAD",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CIPHEAD=1\r\n","OK"))				//显示IP头
		delay_ms(500);
	
  LCD_ShowChinese(43,  128,"登录域名",RED,WHITE,16,0);
	while(M6312_SendCmd(M6312_ONENET_INFO,"CONNECT"))				//连接onenet域名端口
		delay_ms(500);
	
  LCD_ShowChinese(43,  128,"登录成功！",RED,WHITE,16,0);

}


void Send_Cn_message(char*number,char*content)//发送中文短信
{
	char cmd[100];
  LCD_ShowString(43,  128,"AT+CIPCLOSE",RED,WHITE,16,0);//先断开服务器连接
  UsartPrintf(USART2,"AT+CIPCLOSE\r\n");
		delay_ms(200);
  
  LCD_ShowString(43,  128,"AT",RED,WHITE,16,0);
	while(M6312_SendCmd("AT\r\n","OK"))	
		delay_ms(200);
  
  LCD_ShowString(43,  128,"AT+CPIN?",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CPIN?\r\n","READY"));	//没有SIM卡
		delay_ms(200);
  
  LCD_ShowString(43,  128,"AT+CREG?",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CREG?\r\n","0,1"))
	{
		while(strstr((const char*)USART2_RX_BUF,"0,5")==NULL)
		{
       LCD_ShowString(43,  128,"AT+CSQ",RED,WHITE,16,0);
			 while(!M6312_SendCmd("AT+CSQ\r\n","OK"))	
			 {
					memcpy(SIM900_CSQ,USART2_RX_BUF+15,2);
			 }
         return;      //等待附着到网络
		}
	}  

  LCD_ShowString(43,  128,"AT+CMGF=1",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CMGF=1\r\n","OK"))	
		delay_ms(200);
  LCD_ShowString(43,  128,"AT+CSCS",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CSCS=\"UCS2\"\r\n","OK"))
		delay_ms(200);
  LCD_ShowString(43,  128,"AT+CSCA?",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CSCA?\r\n","OK"))	
    delay_ms(200);
  LCD_ShowString(43,  128,"AT+CSMP",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CSMP=17,167,0,8\r\n","OK"))	
    delay_ms(200);
  LCD_ShowString(43,  128,"AT+CMGS",RED,WHITE,16,0);
	sprintf((char*)cmd,"AT+CMGS=\"%s\"\r\n",number);
	while(M6312_SendCmd(cmd,">"))	
    delay_ms(200);
  
	UsartPrintf(USART2,content,strlen(content));
  LCD_ShowString(43,  128,"0x1A",RED,WHITE,16,0);
	USART2->DR=(u32)0x1A;
    delay_ms(200);
  
  LCD_ShowChinese(43,  128,"发送成功！",RED,WHITE,16,0);
}

void Send_En_message(char*number,char*content)//发送英文短信
{
	char cmd[100];

  LCD_ShowString(43,  128,"AT+CMGF=1",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CMGF=1\r\n","OK"))	
		delay_ms(200);
  LCD_ShowString(43,  128,"AT+CSCS",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CSCS=\"GSM\"\r\n","OK"))
		delay_ms(200);
  LCD_ShowString(43,  128,"AT+CSCA?",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CSCA?\r\n","OK"))	
    delay_ms(200);
  LCD_ShowString(43,  128,"AT+CSMP",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CSMP=17,167,0,241\r\n","OK"))	
    delay_ms(200);
  LCD_ShowString(43,  128,"AT+CMGS=",RED,WHITE,16,0);
	sprintf((char*)cmd,"AT+CMGS=\"%s\"\r\n",number);
	while(M6312_SendCmd(cmd,">"))	
    delay_ms(200);
	UsartPrintf(USART2,content,strlen(content));
  LCD_ShowString(43,  128,"0x1A",RED,WHITE,16,0);
	USART2->DR=(u32)0x1A;
    delay_ms(200);
  
  LCD_ShowChinese(43,  128,"发送成功！",RED,WHITE,16,0);
}
void Make_Call(char *number)  //拨打电话
{
	char cmd[20];
  LCD_ShowString(43,  128,"AT+CIPCLOSE",RED,WHITE,16,0);//先断开服务器连接
  UsartPrintf(USART2,"AT+CIPCLOSE\r\n");
		delay_ms(200);
  
  LCD_ShowString(43,  128,"AT",RED,WHITE,16,0);
	while(M6312_SendCmd("AT\r\n","OK"))	
		delay_ms(200);
  
  LCD_ShowString(43,  128,"AT+CPIN?",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CPIN?\r\n","READY"));	//没有SIM卡
		delay_ms(200);
  
  LCD_ShowString(43,  128,"AT+CREG?",RED,WHITE,16,0);
	while(M6312_SendCmd("AT+CREG?\r\n","0,1"))
	{
		while(strstr((const char*)USART2_RX_BUF,"0,5")==NULL)
		{
       LCD_ShowString(43,  128,"AT+CSQ",RED,WHITE,16,0);
			 while(!M6312_SendCmd("AT+CSQ\r\n","OK"))	
			 {
					memcpy(SIM900_CSQ,USART2_RX_BUF+15,2);
			 }
         return;      //等待附着到网络
		}
	}
  
  LCD_ShowChinese(43,  128,"拨打中！",RED,WHITE,16,0);
	sprintf(cmd,"ATD%s;\r\n",number);
	while(M6312_SendCmd(cmd,"OK"))	
    delay_ms(200);
  
  LCD_ShowChinese(43,  128,"拨打成功！",RED,WHITE,16,0);
}


//==========================================================
//	函数名称：	USART2_IRQHandler
//
//	函数功能：	串口2收发中断
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void USART2_IRQHandler(void)
{
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) //接收中断
	{
		if(m6312_cnt >= sizeof(m6312_buf))	m6312_cnt = 0; //防止串口被刷爆
		m6312_buf[m6312_cnt++] = USART2->DR;

		USART_ClearFlag(USART2, USART_FLAG_RXNE);
        
	}

}
