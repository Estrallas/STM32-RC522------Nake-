/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	onenet.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2017-05-08
	*
	*	版本： 		V1.1
	*
	*	说明： 		与onenet平台的数据交互接口层
	*
	*	修改记录：	V1.0：协议封装、返回判断都在同一个文件，并且不同协议接口不同。
	*				V1.1：提供统一接口供应用层使用，根据不同协议文件来封装协议相关的内容。
	************************************************************
	************************************************************
	************************************************************
**/

//单片机头文件
#include "stm32f10x.h"

//网络设备
#include "m6312.h"

//协议文件
#include "onenet.h"
#include "mqttkit.h"

//硬件驱动
#include "usart.h"
#include "delay.h"
#include "led.h"
#include "lcd.h"
//C库
#include <string.h>
#include <stdio.h>

//json库
#include"cjson.h"


#define PROID		"560269"

#define AUTH_INFO	"111"

#define DEVID		"1018786265"


extern unsigned char esp8266_buf[128];
extern u16 CO2Data;         //二氧化碳气体变量
extern short Temperature;   //定义温度变量
extern char ADflag;    //定义薄膜压力传感器的ADC变量
extern char CARflag;    //定义汽车状态的变量;	     		//X轴加速度值暂存
extern _Bool State;

/********阈值********/
extern u16 CO2_H;
extern short Temperature_H;

//==========================================================
//	函数名称：	OneNet_DevLink
//
//	函数功能：	与onenet创建连接
//
//	入口参数：	无
//
//	返回参数：	1-成功	0-失败
//
//	说明：		与onenet平台建立连接
//==========================================================
_Bool OneNet_DevLink(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//协议包

	unsigned char *dataPtr;
	
	_Bool status = 1;
	UsartPrintf(USART_DEBUG, "OneNet_DevLink\r\n"
							"PROID: %s,	AUIF: %s,	DEVID:%s\r\n"
                        , PROID, AUTH_INFO, DEVID);
	if(MQTT_PacketConnect(PROID, AUTH_INFO, DEVID, 256, 0, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
	{
		M6312_SendData(mqttPacket._data, mqttPacket._len);				//上传平台
		dataPtr = M6312_GetIPD(300);									//等待平台响应250

		//如果dataPtr不为空
		if(dataPtr != NULL)
		{
			if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
			{
				switch(MQTT_UnPacketConnectAck(dataPtr))
				{
					case 0:LCD_ShowChinese(43,  128,"连接失败1",RED,WHITE,16,0);status = 0;break;
					
					case 1:LCD_ShowChinese(43,  128,"连接失败1",RED,WHITE,16,0); break;
					case 2:LCD_ShowChinese(43,  128,"连接失败2",RED,WHITE,16,0); break;
					case 3:LCD_ShowChinese(43,  128,"连接失败3",RED,WHITE,16,0); break;
					case 4:LCD_ShowChinese(43,  128,"连接失败4",RED,WHITE,16,0); break;
					case 5:LCD_ShowChinese(43,  128,"连接失败5",RED,WHITE,16,0); break;
					
					default:LCD_ShowChinese(43,  128,"连接失败6",RED,WHITE,16,0);break;
				}
			}
		}
		MQTT_DeleteBuffer(&mqttPacket);								//删包
	}
	else
    LCD_ShowChinese(43,  128,"连接成功！",RED,WHITE,16,0); 
	
	return status;
	
}


//==========================================================
//	函数名称：	OneNet_Ping
//
//	函数功能：	发送心跳
//
//	入口参数：	无
//
//	返回参数：	1-失败	0-成功
//
//	说明：		与onenet平台建立连接
//==========================================================
_Bool OneNet_Ping(void)
{
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//协议包

	unsigned char *dataPtr;
	
	_Bool status = 1;
	
	if(MQTT_PacketPing(&mqttPacket)== 0)
	{
		M6312_SendData(mqttPacket._data, mqttPacket._len);				//上传平台
		dataPtr = M6312_GetIPD(300);									//等待平台响应300
		if(dataPtr != NULL)
		{
			if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_PINGRESP)
			{
				status=0;
			}
		}
		else
		{
			return status;
		}
		MQTT_DeleteBuffer(&mqttPacket);								//删包		
	}
	return status;
}

/*******************************
发送数据内容的重点函数，需改动！！
↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓
*********************************/
unsigned char OneNet_FillBuf(char *buf)//暂时只能发送5条数据了，容量是256字节，现在是251个字节，到极限了
{
	
	char text[64];
	
	memset(text, 0, sizeof(text));
	strcpy(buf, "{\"datastreams\":[");
  
	memset(text, 0, sizeof(text));
	sprintf(text, "{\"id\":\"DATA\",\"datapoints\":[{\"value\":$%d.%d$%d$%d$%d$%d$%d$%d$%d$%d$}]}", Temperature/10,Temperature%10,CO2Data,ADflag,LeiDa,PCin(15),Hongwai,CARflag,Temperature_H,CO2_H);
	strcat(buf, text);  
  
//	memset(text, 0, sizeof(text));
//	sprintf(text, "{\"id\":\"Temperature\",\"datapoints\":[{\"value\":%d.%d}]},", Temperature/10,Temperature%10);
//	strcat(buf, text);
//	
//	memset(text, 0, sizeof(text));
//	sprintf(text, "{\"id\":\"CO2\",\"datapoints\":[{\"value\":%d}]},", co2Data);
//	strcat(buf, text);
//	
//	memset(text, 0, sizeof(text));
//	sprintf(text, "{\"id\":\"ADflag\",\"datapoints\":[{\"value\":%d}]},", ADflag);
//	strcat(buf, text);
//  
//	memset(text, 0, sizeof(text));
//	sprintf(text, "{\"id\":\"BEEP\",\"datapoints\":[{\"value\":%d}]},", PCin(15));
//	strcat(buf, text);
//  
//	memset(text, 0, sizeof(text));
//	sprintf(text, "{\"id\":\"Hongwai\",\"datapoints\":[{\"value\":%d}]}", Hongwai);
//	strcat(buf, text);
  
  
	strcat(buf, "]}");
  
  
	return strlen(buf);

}

//==========================================================
//	函数名称：	OneNet_SendData
//
//	函数功能：	上传数据到平台
//
//	入口参数：	type：发送数据的格式
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNet_SendData(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};												//协议包
	
	char buf[256];
	
	short body_len = 0, i = 0;
	
	UsartPrintf(USART_DEBUG, "Tips:	OneNet_SendData-MQTT\r\n");
	
	memset(buf, 0, sizeof(buf));
	
	body_len = OneNet_FillBuf(buf);																	//获取当前需要发送的数据流的总长度
	
	if(body_len)
	{
		if(MQTT_PacketSaveData(DEVID, body_len, NULL, 1, &mqttPacket) == 0)							//封包
		{ 
			for(; i < body_len; i++)
				mqttPacket._data[mqttPacket._len++] = buf[i];					
			M6312_SendData(mqttPacket._data, mqttPacket._len);										//上传数据到平台
			UsartPrintf(USART_DEBUG, "Send %d Bytes\r\n", mqttPacket._len);
			
			MQTT_DeleteBuffer(&mqttPacket);															//删包
		}
		else
			State=1,LCD_ShowChinese(43,  128,"上传失败！",RED,WHITE,16,0);
	}
	
}

//==========================================================
//	函数名称：	OneNet_RevPro
//
//	函数功能：	平台返回数据检测
//
//	入口参数：	dataPtr：平台返回的数据
//
//	返回参数：	无
//
//	说明：	判断云平台下发的命令，并执行相应操作(需改动)。
//==========================================================
void OneNet_RevPro(unsigned char *cmd)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};								//协议包
	
	char *req_payload = NULL;
	char *cmdid_topic = NULL;
	
	unsigned short req_len = 0;
	
	unsigned char type = 0;
	
	short result = 0;

	char *dataPtr = NULL;
	char numBuf[10];
	int num = 0;

	  cJSON *json1, *json_value1;
      cJSON *json2, *json_value2;
      cJSON *json3, *json_value3;

  
	type = MQTT_UnPacketRecv(cmd);
	switch(type)
	{
		case MQTT_PKT_CMD:															//命令下发
			
			result = MQTT_UnPacketCmd(cmd, &cmdid_topic, &req_payload, &req_len);	//解出topic和消息体
			if(result == 0)
			{
				UsartPrintf(USART_DEBUG, "cmdid: %s, req: %s, req_len: %d\r\n", cmdid_topic, req_payload, req_len);
				
				// 对数据包req_payload进行JSON格式解析：
        
		/***以下重点改动↓↓↓↓↓↓↓***/
				json1 = cJSON_Parse(req_payload);
				if (!json1)//如果json内容为空，则打印错误信息
					State=1;
				else
				{
					json_value1 = cJSON_GetObjectItem(json1 , "BEEP");//提取对应属性的数值
					if((json_value1->valueint)==1)
						BEEP=1;//开启蜂鸣器
					else if((json_value1->valueint)==0)			
					    BEEP=0;//关闭蜂鸣器
				}
        /**************↑↑↑↑↑↑↑***/
        
	   /***以下重点改动↓↓↓↓↓↓↓***/
				json2 = cJSON_Parse(req_payload);
				if (!json2)//如果json内容为空，则打印错误信息
					State=1;
				else
				{
					json_value2 = cJSON_GetObjectItem(json2 , "CO2");//提取对应属性的数值
					 CO2_H=json_value2->valueint;
                    
				}
        /**************↑↑↑↑↑↑↑***/
       
		/***以下重点改动↓↓↓↓↓↓↓***/
				json3 = cJSON_Parse(req_payload);
				if (!json3)//如果json内容为空，则打印错误信息
					State=1;
				else
				{
					json_value3 = cJSON_GetObjectItem(json3 , "Temperature");//提取对应属性的数值
					Temperature_H=json_value3->valueint;
				}
        /**************↑↑↑↑↑↑↑***/     

    
        
				if(MQTT_PacketCmdResp(cmdid_topic, req_payload, &mqttPacket) == 0)	//命令回复组包
				{
					UsartPrintf(USART_DEBUG, "Tips:	Send CmdResp\r\n");
					
					M6312_SendData(mqttPacket._data, mqttPacket._len);				//回复命令
					MQTT_DeleteBuffer(&mqttPacket);									//删包
				}
				cJSON_Delete(json1);//释放位于堆中cJSON结构体内存
				cJSON_Delete(json2);
				cJSON_Delete(json3);
			}
		
		break;
			
		case MQTT_PKT_PUBACK:														//发送Publish消息，平台回复的Ack
		
			if(MQTT_UnPacketPublishAck(cmd) == 0)
              LCD_ShowChinese(43,  128,"发送数据中",RED,WHITE,16,0);
            else State=1, LCD_ShowChinese(43,  128,"发送 错误",RED,WHITE,16,0);
            
			
		break;
              
		default:LCD_ShowChinese(43,  128,"上传失败！",RED,WHITE,16,0);
			State=1,result = -1;
		break;
	}
	
	M6312_Clear();										//清空缓存
	
	if(result == -1)
		 return;
	
	dataPtr = strchr(req_payload, '}');					//搜索'}'

	if(dataPtr != NULL && result != -1)					//如果找到了
	{
		dataPtr++;
		
		while(*dataPtr >= '0' && *dataPtr <= '9')		//判断是否是下发的命令控制数据
		{
			numBuf[num++] = *dataPtr++;
		}
		numBuf[num] = 0;
		
		num = atoi((const char *)numBuf);				//转为数值形式
	}

	if(type == MQTT_PKT_CMD || type == MQTT_PKT_PUBLISH)
	{
		MQTT_FreeBuffer(cmdid_topic);
		MQTT_FreeBuffer(req_payload);
	}

}