#include "stm32f10x.h"                  // Device header
#include "delay.h"
#include "sys.h"
#include "OLED.h"
#include "rc522.h"
#include "rc522Conf.h"
#include "Servo.h"
#include "lcd1602.h"

//C库
#include <string.h>
#include "stdio.h"
#include "math.h"
#include "stdlib.h"

char oled_str[50];
u8 ID[4];
u8 status = 0;

// 卡号
u8 CardA_UID[4] = {0xDD, 0x9A, 0xF0, 0x06}; 

//验证
u8 Check_CardA(u8 *uid)
{
    if(uid[0]==CardA_UID[0] && uid[1]==CardA_UID[1]
    && uid[2]==CardA_UID[2] && uid[3]==CardA_UID[3])
    {
        return 1;
    }
    return 0;
}

int main(void)
{
    delay_init();
//    OLED_Init();
		PCA9685_Init();
    OLED_Clear(0);
    rc522Init();
		LCD1602_Init();
 
		LCD1602_Write_String(0,0,"    HELLO!!!   ");//?????
		LCD1602_Write_String(0,1,"                ");

//		PCA9685_SetAngle(1, 0.0f);
	

		int i=0;

    while(1)
    {
			
				LCD1602_WriteCom(0xC6);
				LCD1602_WriteData(i+0x30);

        OLED_ShowText(0,0,(u8*)"读卡中...",0);
        status = readCard(ID);

        if(status == 0)
        {
            sprintf(oled_str,"%d%d%d%d",ID[0],ID[1],ID[2],ID[3]);
            OLED_ShowText(0,2,(u8*)oled_str,0);

            if(Check_CardA(ID))
            {
                OLED_ShowText(0,4,(u8*)"A      ",0);
//							PCA9685_SetAngle(1, 90.0f);
 
            }
            else
            {
                OLED_ShowText(0,4,(u8*)"ERROR",0);
            }
        }
//        else
//        {
//            OLED_ShowText(0,2,(u8*)"          ",0);
//            OLED_ShowText(0,4,(u8*)"          ",0);
            
//            // 重置
//            ID[0] = 0x00;
//            ID[1] = 0x00;
//            ID[2] = 0x00;
//            ID[3] = 0x00;
//        }

    }
}
