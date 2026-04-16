#ifndef _M6312_H_
#define _M6312_H_


#define REV_OK		0	//接收完成标志
#define REV_WAIT	1	//接收未完成标志


void M6312_Init(void);

void M6312_Clear(void);

void M6312_SendData(unsigned char *data, unsigned short len);

unsigned char *M6312_GetIPD(unsigned short timeOut);

void Send_Cn_message(char*number,char*content);//发送中文短信

void Send_En_message(char*number,char*content);//发送英文短信

void Make_Call(char *number);  //拨打电话
#endif
