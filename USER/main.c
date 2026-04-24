#include "stm32f10x.h"                  // Device header
#include "delay.h"
#include "sys.h"
#include "OLED.h"
#include "OLED_Data.h"
#include "rc522.h"
#include "rc522Conf.h"
#include "Servo.h"
#include "button4_4.h"
#include "user_logic.h"
#include "stm32f10x_flash.h"
#include "timer_key.h"


#include <string.h>
#include "stdio.h"
#include "math.h"
#include "stdlib.h"

char oled_str[50];
      
   
u8 ID[4];
u8 status = 0;

u8 CardA_UID[4] = {0xDD, 0x9A, 0xF0, 0x06}; 

u8 Check_CardA(u8 *uid)
{
    if(uid[0]==CardA_UID[0] && uid[1]==CardA_UID[1]
    && uid[2]==CardA_UID[2] && uid[3]==CardA_UID[3])
    {
        return 1;
    }
    return 0;
}


u8 value[6]={0,0,0,0,0,0};

unsigned int key_masks = 0;

int main(void)
{

    delay_init();
    Button4_4_Init();
    TimerKey_Init();
    OLED_Init();
    PCA9685_Init();
    OLED_Clear();
    rc522Init();
    UserLogic_Init();
    



		PCA9685_SetAngle(0, 90.0f);
	


    while(1)
    {
                // 从 TIM2 去抖模块读取按键事件
                int k = TimerKey_GetKey();
                if (k != 0) {
                    UserLogic_HandleKey(k);
                }
                // check RFID
                u8 idbuf[4];
                if (readCard(idbuf) == 0) {
                    UserLogic_HandleRFID(idbuf);
                }
//        OLED_ShowText(0,0,(u8*)"读取...",0);
//        status = readCard(ID);
//        if(status == 0)
//        {
//            sprintf(oled_str,"%d%d%d%d",ID[0],ID[1],ID[2],ID[3]);
//            OLED_ShowText(0,2,(u8*)oled_str,0);

//            if(Check_CardA(ID))
//            {
////                OLED_ShowText(0,4,(u8*)"A      ",0);
////							PCA9685_SetAngle(1, 90.0f);
// 
//            }
//            else
//            {
//                OLED_ShowText(0,4,(u8*)"ERROR",0);
//            }
//        }

    }
}
