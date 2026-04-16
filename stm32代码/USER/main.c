#include "stm32f10x.h"                  // Device header
#include "delay.h"
#include "sys.h"
#include "OLED.h"
#include "rc522.h"
#include "rc522Conf.h"
#include "Servo.h"

//C��
#include <string.h>
#include "stdio.h"
#include "math.h"
#include "stdlib.h"


char oled_str[50];
u8 ID[4];
u8 status = 0; 
static int tem_angle=0;
int main(void)
{
	delay_init();	    	 //��ʱ������ʼ��	  
	PCA9685_Init();
    OLED_Init();
    OLED_Clear(0);//����

    // Startup sweep to verify PCA9685 output and servo wiring.



	/*RC522����ģ���ʼ��*/
    rc522Init();
	/*RC522����ģ���ʼ��*/
    
    while(1)
    {     
					for(int i=0;i<12;i++)
	{
		PCA9685_SetAngle(i,(float)tem_angle);
	}
        /***********������*************/
        OLED_ShowText(0,0,(u8*)"LOADING...",0);
        status=readCard(ID);//�����ź���
        if(status==0)
        {
            sprintf(oled_str,"ID:%d%d%d%d  ",ID[0],ID[1],ID[2],ID[3]);
            OLED_ShowText(0,2,(u8*)oled_str,0); 
        }
    }
}


