#include "oled.h"
#include "stdlib.h"
#include "oledfont.h"  	 
#include "delay.h"

//OLED���Դ�
//��Ÿ�ʽ����.
//[0]0 1 2 3 ... 127
//[1]0 1 2 3 ... 127
//[2]0 1 2 3 ... 127
//[3]0 1 2 3 ... 127
//[4]0 1 2 3 ... 127
//[5]0 1 2 3 ... 127
//[6]0 1 2 3 ... 127
//[7]0 1 2 3 ... 127
#if (OLED_MODE==OLED_IIC)
static void OLED_I2C_BusRecover(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    u8 i;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_I2C1, OLED_I2C_REMAP);

    GPIO_InitStructure.GPIO_Pin = OLED_SCL_Pin | OLED_SDA_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_SCL_GPIO_Port, &GPIO_InitStructure);

    GPIO_SetBits(OLED_SCL_GPIO_Port, OLED_SCL_Pin | OLED_SDA_Pin);
    delay_us(5);

    if (GPIO_ReadInputDataBit(OLED_SDA_GPIO_Port, OLED_SDA_Pin) == Bit_RESET)
    {
        for (i = 0; i < 16; i++)
        {
            GPIO_ResetBits(OLED_SCL_GPIO_Port, OLED_SCL_Pin);
            delay_us(5);
            GPIO_SetBits(OLED_SCL_GPIO_Port, OLED_SCL_Pin);
            delay_us(5);

            if (GPIO_ReadInputDataBit(OLED_SDA_GPIO_Port, OLED_SDA_Pin) == Bit_SET)
            {
                break;
            }
        }
    }

    GPIO_ResetBits(OLED_SDA_GPIO_Port, OLED_SDA_Pin);
    delay_us(5);
    GPIO_SetBits(OLED_SCL_GPIO_Port, OLED_SCL_Pin);
    delay_us(5);
    GPIO_SetBits(OLED_SDA_GPIO_Port, OLED_SDA_Pin);
    delay_us(5);
}

static void OLED_I2C_ResetPeripheral(void)
{
    I2C_SoftwareResetCmd(OLED_I2C, ENABLE);
    delay_us(5);
    I2C_SoftwareResetCmd(OLED_I2C, DISABLE);
    I2C_Cmd(OLED_I2C, ENABLE);
}

static u8 OLED_I2C_WaitEvent(uint32_t event)
{
    u32 timeout = 0x3FFFF;

    while (I2C_CheckEvent(OLED_I2C, event) != SUCCESS)
    {
        if (timeout-- == 0)
        {
            return 1;
        }
    }

    return 0;
}

static u8 OLED_I2C_WaitBusIdle(void)
{
    u32 timeout = 0x3FFFF;

    while (I2C_GetFlagStatus(OLED_I2C, I2C_FLAG_BUSY) == SET)
    {
        if (timeout-- == 0)
        {
            return 1;
        }
    }

    return 0;
}

static u8 OLED_I2C_WriteByte(u8 control, u8 data)
{
    if (OLED_I2C_WaitBusIdle())
    {
        goto i2c_error;
    }

    I2C_GenerateSTART(OLED_I2C, ENABLE);
    if (OLED_I2C_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT))
    {
        goto i2c_error;
    }

    I2C_Send7bitAddress(OLED_I2C, OLED_I2C_ADDR, I2C_Direction_Transmitter);
    if (OLED_I2C_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        goto i2c_error;
    }

    I2C_SendData(OLED_I2C, control);
    if (OLED_I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
        goto i2c_error;
    }

    I2C_SendData(OLED_I2C, data);
    if (OLED_I2C_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
        goto i2c_error;
    }

    I2C_GenerateSTOP(OLED_I2C, ENABLE);
    return 0;

i2c_error:
    I2C_ClearFlag(OLED_I2C, I2C_FLAG_AF);
    I2C_GenerateSTOP(OLED_I2C, ENABLE);
    OLED_I2C_ResetPeripheral();
    return 1;
}
//��SSD1106д��һ���ֽڡ�
//dat:Ҫд�������/����
//cmd:����/�����־ 0,��ʾ����;1,��ʾ����;
void OLED_WR_Byte(u8 dat,u8 cmd)
{
    u8 control = cmd ? 0x40 : 0x00;
    if (OLED_I2C_WriteByte(control, dat) != 0)
    {
        (void)OLED_I2C_WriteByte(control, dat);
    }
}
#elif  (OLED_MODE==OLED_SPI)
//��SSD1106д��һ���ֽڡ�
//dat:Ҫд�������/����
//cmd:����/�����־ 0,��ʾ����;1,��ʾ����;
void OLED_WR_Byte(u8 dat,u8 cmd)
{
    u8 i;
    if(cmd)
    {
        OLED_DC_Set();
    }
    else
    {
        OLED_DC_Clr();
    }
    OLED_CS_Clr();
    for(i=0; i<8; i++)
    {
        OLED_SCLK_Clr();
        if(dat&0x80)
        {
            OLED_SDIN_Set();
        }
        else
        {
            OLED_SDIN_Clr();
        }
        OLED_SCLK_Set();
        dat<<=1;
    }
    OLED_CS_Set();
    OLED_DC_Set();
}
#endif
void OLED_Set_Pos(u8 x, u8 y)
{
    OLED_WR_Byte(0xb0+y,OLED_CMD);
    OLED_WR_Byte(((x&0xf0)>>4)|0x10,OLED_CMD);
    OLED_WR_Byte((x&0x0f)|0x01,OLED_CMD);
}
//����OLED��ʾ
void OLED_Display_On(void)
{
    OLED_WR_Byte(0X8D,OLED_CMD);  //SET DCDC����
    OLED_WR_Byte(0X14,OLED_CMD);  //DCDC ON
    OLED_WR_Byte(0XAF,OLED_CMD);  //DISPLAY ON
}
//�ر�OLED��ʾ
void OLED_Display_Off(void)
{
    OLED_WR_Byte(0X8D,OLED_CMD);  //SET DCDC����
    OLED_WR_Byte(0X10,OLED_CMD);  //DCDC OFF
    OLED_WR_Byte(0XAE,OLED_CMD);  //DISPLAY OFF
}
//��������,������,������Ļ�Ǻ�ɫ��!��û����һ��!!!
void OLED_Clear(u8 mode)
{
    u8 i,n;
    for(i=0; i<8; i++)
    {
        OLED_WR_Byte (0xb0+i,OLED_CMD);    //����ҳ��ַ��0~7��
        OLED_WR_Byte (0x00,OLED_CMD);      //������ʾλ�á��е͵�ַ
        OLED_WR_Byte (0x10,OLED_CMD);      //������ʾλ�á��иߵ�ַ
        for(n=0; n<128; n++)
        {
            OLED_WR_Byte(0,OLED_DATA);
        }
    } //������ʾ
}
/********************************************************************************
* @��������         OLED_ShowChar
* @����������       ��ָ��λ����ʾռ��8*16�ĵ���ASCII�ַ�
* @���������
                    ������    ��������  ��������
                    @x��      u8         �����꣬0~127
                    @y��      u8         �����꣬0~7
                    @chr��    u8         Ҫ��ʾ��ASCII�ַ�
                    @flag��   u8         ���ױ�־����0ʱ������ʾ
* @����ֵ��         void
* @������
********************************************************************************/
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 flag)
{
    unsigned char c=0,i=0;
    c=chr-' ';//�õ�ƫ�ƺ��ֵ
    if(x>Max_Column-1)
    {
        x=0;
        y=y+2;
    }
    OLED_Set_Pos(x,y);
    for(i=0; i<8; i++)
    {
        if(flag==1)
        {
            OLED_WR_Byte(~F8X16[c*16+i],OLED_DATA);
        }
        else
        {
            OLED_WR_Byte(F8X16[c*16+i],OLED_DATA);
        }
    }
    OLED_Set_Pos(x,y+1);
    for(i=0; i<8; i++)
    {
        if(flag==1)
        {
            OLED_WR_Byte(~F8X16[c*16+i+8],OLED_DATA);
        }
        else
        {
            OLED_WR_Byte(F8X16[c*16+i+8],OLED_DATA);
        }
    }
}
//m^n����
u32 oled_pow(u8 m,u8 n)
{
    u32 result=1;
    while(n--)
    {
        result*=m;
    }
    return result;
}

/********************************************************************************
* @��������         OLED_ShowCHineseWord
* @����������       ��ָ��λ����ʾռ��16*16�ĵ��� ����
* @���������
                    ������    ��������  ��������
                    @x��      u8         �����꣬0~127
                    @y��      u8         �����꣬0~7
                    @str��    u8*        Ҫ��ʾ�ĺ���
                    @flag��   u8         ���ױ�־����0ʱ������ʾ
* @����ֵ��         void
* @������
********************************************************************************/
void OLED_ShowCHineseWord(u8 x,u8 y,u8* str,u8 flag)
{
    u8 t=0;
    u16 index;
    for(index=0; index<sizeof(CHINESE_CODE)/35; index++)
    {
        if(CHINESE_CODE[index].Text[0] == str[0]&&CHINESE_CODE[index].Text[1] == str[1])//�ԱȺ�������λ��
        {
            OLED_Set_Pos(x,y);  //����OLED���λ��
            for(t=0; t<16; t++)//��д�����ϰ벿������
            {
                if(flag==0)
                {
                    OLED_WR_Byte(CHINESE_CODE[index].Code[t],OLED_DATA);
                }
                else
                {
                    OLED_WR_Byte(~CHINESE_CODE[index].Code[t],OLED_DATA);
                }
            }
            OLED_Set_Pos(x,y+1);//����OLED���λ��
            for(t=0; t<16; t++) //��д�����°벿������
            {
                if(flag==0)
                {
                    OLED_WR_Byte(CHINESE_CODE[index].Code[t+16],OLED_DATA);
                }
                else
                {
                    OLED_WR_Byte(~CHINESE_CODE[index].Code[t+16],OLED_DATA);
                }
            }
            break;
        }
    }
}

/********************************************************************************
* @��������         OLED_ShowText
* @����������       ��ָ��λ����ʾ��Ӣ���ַ���
* @���������
                    ������    ��������  ��������
                    @x��      u8         �����꣬0~127
                    @y��      u8         �����꣬0~7
                    @str��    u8*        Ҫ��ʾ���ַ���
                    @flag��   u8         ���ױ�־����0ʱ������ʾ
* @����ֵ��         void
* @������
********************************************************************************/
void OLED_ShowText(u8 x,u8 y,u8* str,u8 flag)
{
    u8 tempstr[3] = {'\0'};
	
    while((*str)!='\0')
    {
        if((*str)&0x80)//�ж���һ���ַ������Ļ���Ӣ��
        {
            tempstr[0]=*str++;
            tempstr[1]=*str++;
            OLED_ShowCHineseWord(x,y,tempstr,flag);
            x+=16;
            if(x>128-16)
            {
                y++;    //�޸ĵ�ַ
                y++;
                x=0;
            }
        }
        else
        {
            OLED_ShowChar(x,y,*str++,flag);
            x+=8;
			if(x>128-8)
			{
				y++;    //�޸ĵ�ַ
				y++;
				x=0;
			}
		}
	}
//	HAL_TIM_Base_Start_IT(&htim1);
//	HAL_TIM_Base_Start_IT(&htim2);	
}
/***********������������ʾ��ʾBMPͼƬ128��64��ʼ������(x,y),x�ķ�Χ0��127��yΪҳ�ķ�Χ0��7*****************/
void OLED_DrawBMP(unsigned char x0, unsigned char y0,unsigned char x1, unsigned char y1,const unsigned char BMP[])
{
    unsigned int j=0;
    unsigned char x,y;
    if(y1%8==0)
    {
        y=y1/8;
    }
    else
    {
        y=y1/8+1;
    }
    for(y=y0; y<y1; y++)
    {
        OLED_Set_Pos(x0,y);
        for(x=x0; x<x1; x++)
        {
            OLED_WR_Byte(BMP[j++],OLED_DATA);
        }
    }
}

/**
 * @brief ��ʾһ���ַ���
 * @param x,y : ��ʼ������(x:0~127, y:0~7)
 * @param *p : �ַ�����ʼ��ַ
 * @param size : �ַ���С(�ֺ�16)
 * @return None
 */
void OLED_ShowString(u8 x, u8 y, u8 *p, u8 size)
{
    // ����size��������������֧��16����
    while(*p != '\0')
    {
        OLED_ShowChar(x, y, *p, 0);
        x += 8;
        p++;
        if(x > 120) {
            x = 0;
            y += 2;
        }
    }
}

/**
 * @brief ��ʾ����
 * @param x,y : ��ʼ����
 * @param num : Ҫ��ʾ������
 * @param len : ���ֵ�λ��
 * @return None
 */
void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len)
{
    u8 t, temp;
    for(t = 0; t < len; t++)
    {
        temp = (num / oled_pow(10, len - t - 1)) % 10;
        OLED_ShowChar(x + 8 * t, y, temp + '0', 0);
    }
}

/**
 * @brief ��ʾ����
 * @param x,y : ��ʼ����
 * @param no : ������oledfont.h�е�����
 * @return None
 */
void OLED_ShowChinese(u8 x, u8 y, u8 no)
{
    u8 t, adder = 0;
    u8 str[3];
    
    // ����������CHINESE_CODE���ҵ���Ӧ����
    str[0] = CHINESE_CODE[no].Text[0];
    str[1] = CHINESE_CODE[no].Text[1];
    str[2] = '\0';
    
    OLED_ShowCHineseWord(x, y, str, 0);
}

//��ʼ��SSD1306
void OLED_Init(void)
{
	const u8 OLED_ConfigCmd[]=
	{
		0xAE,    //display off
		0x20,    //Set Memory Addressing Mode	
		0x10,    //00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
		0xb0,    //Set Page Start Address for Page Addressing Mode,0-7
		0xc8,    //Set COM Output Scan Direction
		0x00,    //---set low column address
		0x10,    //---set high column address
		0x40,    //--set start line address
		0x81,    //--set contrast control register
		0xdf,    //���ȵ��� 0x00~0xff
		0xa1,    //--set segment re-map 0 to 127
		0xa6,    //--set normal display
		0xa8,    //--set multiplex ratio(1 to 64)
		0x3f,    //
		0xa4,    //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
		0xd3,    //-set display offset
		0x00,    //-not offset
		0xd5,    //--set display clock divide ratio/oscillator frequency
		0xf0,    //--set divide ratio
		0xd9,    //--set pre-charge period
		0x22,    //
		0xda,    //--set com pins hardware configuration
		0x12,    
		0xdb,    //--set vcomh
		0x20,    //0x20,0.77xVcc
		0x8d,    //--set DC-DC enable
		0x14,    //
		0xaf,    //--turn on oled panel
	};	
#if (OLED_MODE==OLED_IIC)
	I2C_InitTypeDef I2C_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    // ʹ��GPIOB/AFIO/I2C1ʱ��
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

    // I2C1����PB8/PB9
    GPIO_PinRemapConfig(GPIO_Remap_I2C1, OLED_I2C_REMAP);

    // ���豸����ʱ�������ܻᵼ���ܾ�ռ���ߣ��ȳ��Խ����ߣ��ٳ�ʼ��I2C
    OLED_I2C_BusRecover();

    // SCL/SDA������Ϊ���ù©���
    GPIO_InitStructure.GPIO_Pin = OLED_SCL_Pin | OLED_SDA_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_SCL_GPIO_Port, &GPIO_InitStructure);

    I2C_DeInit(OLED_I2C);
    I2C_InitStructure.I2C_ClockSpeed = OLED_I2C_SPEED;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Disable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(OLED_I2C, &I2C_InitStructure);
    I2C_Cmd(OLED_I2C, ENABLE);
    
#else
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // ʹ����ӦGPIOʱ��
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE, ENABLE);
    
    // ����SCL����
    GPIO_InitStructure.GPIO_Pin = OLED_SCL_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_SCL_GPIO_Port, &GPIO_InitStructure);
    
    // ����SDA����
    GPIO_InitStructure.GPIO_Pin = OLED_SDA_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_SDA_GPIO_Port, &GPIO_InitStructure);
    
    // ����CS����
    GPIO_InitStructure.GPIO_Pin = OLED_CS_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_CS_GPIO_Port, &GPIO_InitStructure);
    
    // ����DC����
    GPIO_InitStructure.GPIO_Pin = OLED_DC_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_DC_GPIO_Port, &GPIO_InitStructure);
    
    // ����RES����
    GPIO_InitStructure.GPIO_Pin = OLED_RES_Pin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_RES_GPIO_Port, &GPIO_InitStructure);
    
    // �������ų�ʼ��ƽ
    GPIO_SetBits(OLED_SCL_GPIO_Port, OLED_SCL_Pin);
    GPIO_SetBits(OLED_SDA_GPIO_Port, OLED_SDA_Pin);
    GPIO_SetBits(OLED_CS_GPIO_Port, OLED_CS_Pin);
    GPIO_SetBits(OLED_DC_GPIO_Port, OLED_DC_Pin);
    GPIO_SetBits(OLED_RES_GPIO_Port, OLED_RES_Pin);
    
    // ��λOLED
    OLED_RST_Set();
    delay_ms(100);
    OLED_RST_Clr();
    delay_ms(100);
    OLED_RST_Set();
#endif
	delay_ms(100); //�������ʱ����Ҫ
#if 1	
	for(u8 i=0;i<sizeof(OLED_ConfigCmd)/sizeof(OLED_ConfigCmd[0]);i++)
	{
		OLED_WR_Byte(OLED_ConfigCmd[i],OLED_CMD);	
	}
#else
	OLED_WR_Byte(0xAE,OLED_CMD); //display off                                                                                         
	OLED_WR_Byte(0x20,OLED_CMD);	//Set Memory Addressing Mode	                                                                        
	OLED_WR_Byte(0x10,OLED_CMD);	//00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
	OLED_WR_Byte(0xb0,OLED_CMD);	//Set Page Start Address for Page Addressing Mode,0-7                                                 
	OLED_WR_Byte(0xc8,OLED_CMD);	//Set COM Output Scan Direction                                                                       
	OLED_WR_Byte(0x00,OLED_CMD); //---set low column address                                                                          
	OLED_WR_Byte(0x10,OLED_CMD); //---set high column address                                                                         
	OLED_WR_Byte(0x40,OLED_CMD); //--set start line address                                                                           
	OLED_WR_Byte(0x81,OLED_CMD); //--set contrast control register                                                                    
	OLED_WR_Byte(0xff,OLED_CMD); //���ȵ��� 0x00~0xff                                                                                 
	OLED_WR_Byte(0xa1,OLED_CMD); //--set segment re-map 0 to 127                                                                      
	OLED_WR_Byte(0xa6,OLED_CMD); //--set normal display                                                                               
	OLED_WR_Byte(0xa8,OLED_CMD); //--set multiplex ratio(1 to 64)                                                                     
	OLED_WR_Byte(0x3F,OLED_CMD); //                                                                                                   
	OLED_WR_Byte(0xa4,OLED_CMD); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content                                    
	OLED_WR_Byte(0xd3,OLED_CMD); //-set display offset                                                                                
	OLED_WR_Byte(0x00,OLED_CMD); //-not offset                                                                                        
	OLED_WR_Byte(0xd5,OLED_CMD); //--set display clock divide ratio/oscillator frequency                                              
	OLED_WR_Byte(0xf0,OLED_CMD); //--set divide ratio                                                                                 
	OLED_WR_Byte(0xd9,OLED_CMD); //--set pre-charge period                                                                            
	OLED_WR_Byte(0x22,OLED_CMD); //                                                                                                   
	OLED_WR_Byte(0xda,OLED_CMD); //--set com pins hardware configuration                                                              
	OLED_WR_Byte(0x12,OLED_CMD);                                                                                                      
	OLED_WR_Byte(0xdb,OLED_CMD); //--set vcomh                                                                                        
	OLED_WR_Byte(0x20,OLED_CMD); //0x20,0.77xVcc                                                                                      
	OLED_WR_Byte(0x8d,OLED_CMD); //--set DC-DC enable                                                                                 
	OLED_WR_Byte(0x14,OLED_CMD); //                                                                                                   
	OLED_WR_Byte(0xaf,OLED_CMD); //--turn on oled panel                                                                               
#endif
	OLED_Clear(0);
}











