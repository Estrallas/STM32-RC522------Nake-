#ifndef __USART_H
#define __USART_H
#include "sys.h" 

#define USART_REC_LEN  			200  	//定义最大接收字节数 200
#define USART_DEBUG		USART1		//调试打印所使用的串口组
#define USART2_MAX_RECV_LEN		1024				//最大接收缓存字节数

//定义数组长度
#define GPS_Buffer_Length 80
#define UTCTime_Length 11
#define latitude_Length 11
#define N_S_Length 2
#define longitude_Length 12
#define E_W_Length 2 
typedef struct SaveData 
{
	char GPS_Buffer[GPS_Buffer_Length];
	char isGetData;		//是否获取到GPS数据
	char isParseData;	//是否解析完成
	char UTCTime[UTCTime_Length];		//UTC时间
	char latitude[latitude_Length];		//纬度
	char N_S[N_S_Length];		//N/S
	char longitude[longitude_Length];		//经度
	char E_W[E_W_Length];		//E/W
	char isUsefull;		//定位信息是否有效
} _SaveData;


extern u8  USART2_RX_BUF[USART2_MAX_RECV_LEN]; 		//接收缓冲,最大USART2_MAX_RECV_LEN字节
extern u16 USART2_RX_STA;   						//接收数据状态
extern u8 USART2_RX_REC_ATCOMMAD;



//如果想串口中断接收，请不要注释以下宏定义
void uart_init(u32 bound);

void Usart2_Init(unsigned int baud);

void Usart_SendString(USART_TypeDef *USARTx, unsigned char *str, unsigned short len);

void UsartPrintf(USART_TypeDef *USARTx, char *fmt,...);

void USART3_Init(u32 bound);


//GPS函数声明
void errorLog(int num);
void parseGpsBuffer(void);
void printGpsBuffer(void);
void CLR_Buf(void);
u8 Hand(char *a);
void clrStruct(void);
#endif


