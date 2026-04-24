#include "button4_4.h"
#include "delay.h"


static char KEY_ROW_SCAN(void)
{
    uint8_t Key_row = 0xff;
    // 第一次采样
    Key_row = (GPIO_ReadInputDataBit(BUTTON_COL1_GPIO_PORT, BUTTON_COL1_GPIO_PIN) << 3);
    Key_row |= (GPIO_ReadInputDataBit(BUTTON_COL2_GPIO_PORT, BUTTON_COL2_GPIO_PIN) << 2);
    Key_row |= (GPIO_ReadInputDataBit(BUTTON_COL3_GPIO_PORT, BUTTON_COL3_GPIO_PIN) << 1);
    Key_row |= (GPIO_ReadInputDataBit(BUTTON_COL4_GPIO_PORT, BUTTON_COL4_GPIO_PIN));

  
            switch (Key_row)
            {
                case 0x07: return 1; // column1
                case 0x0b: return 2; // column2
                case 0x0d: return 3; // column3
                case 0x0e: return 4; // column4
                default:   return 0;
            

    }
}

char Key_Num = 0;       // 1-16
char key_row_num = 0;   // row index

int Button4_4_Scan(void)
{
    // Row 1
    GPIO_ResetBits(BUTTON_ROW1_GPIO_PORT, BUTTON_ROW1_GPIO_PIN);
    key_row_num = KEY_ROW_SCAN();
    if (key_row_num != 0)
    {
        // 等待按键释放（带短延时防抖），避免长按造成多次触发
        while (KEY_ROW_SCAN() != 0) { delay_ms(10); }
        Key_Num = 0 + key_row_num;
    }
    GPIO_SetBits(BUTTON_ROW1_GPIO_PORT, BUTTON_ROW1_GPIO_PIN);

    // Row 2
    GPIO_ResetBits(BUTTON_ROW2_GPIO_PORT, BUTTON_ROW2_GPIO_PIN);
    key_row_num = KEY_ROW_SCAN();
    if (key_row_num != 0)
    {
        while (KEY_ROW_SCAN() != 0) { delay_ms(10); }
        Key_Num = 4 + key_row_num;
    }
    GPIO_SetBits(BUTTON_ROW2_GPIO_PORT, BUTTON_ROW2_GPIO_PIN);

    // Row 3
    GPIO_ResetBits(BUTTON_ROW3_GPIO_PORT, BUTTON_ROW3_GPIO_PIN);
    key_row_num = KEY_ROW_SCAN();
    if (key_row_num != 0)
    {
        while (KEY_ROW_SCAN() != 0) { delay_ms(10); }
        Key_Num = 8 + key_row_num;
    }
    GPIO_SetBits(BUTTON_ROW3_GPIO_PORT, BUTTON_ROW3_GPIO_PIN);

    // Row 4
    GPIO_ResetBits(BUTTON_ROW4_GPIO_PORT, BUTTON_ROW4_GPIO_PIN);
    key_row_num = KEY_ROW_SCAN();
    if (key_row_num != 0)
    {
        while (KEY_ROW_SCAN() != 0) { delay_ms(10); }
        Key_Num = 12 + key_row_num;
    }
    GPIO_SetBits(BUTTON_ROW4_GPIO_PORT, BUTTON_ROW4_GPIO_PIN);

    return Key_Num;
}

void Button4_4_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(BUTTON_GPIO_CLK, ENABLE);

    // Rows (PA0..PA3) -> Output Push-Pull
    GPIO_InitStructure.GPIO_Pin = BUTTON_ROW1_GPIO_PIN | BUTTON_ROW2_GPIO_PIN | BUTTON_ROW3_GPIO_PIN | BUTTON_ROW4_GPIO_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(BUTTON_ROW1_GPIO_PORT, &GPIO_InitStructure);

    // Columns (PA4..PA7) -> Input Pull-Up
    GPIO_InitStructure.GPIO_Pin = BUTTON_COL1_GPIO_PIN | BUTTON_COL2_GPIO_PIN | BUTTON_COL3_GPIO_PIN | BUTTON_COL4_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(BUTTON_COL1_GPIO_PORT, &GPIO_InitStructure);

    // Set all rows to high (idle)
    GPIO_SetBits(BUTTON_ROW1_GPIO_PORT, BUTTON_ROW1_GPIO_PIN);
    GPIO_SetBits(BUTTON_ROW2_GPIO_PORT, BUTTON_ROW2_GPIO_PIN);
    GPIO_SetBits(BUTTON_ROW3_GPIO_PORT, BUTTON_ROW3_GPIO_PIN);
    GPIO_SetBits(BUTTON_ROW4_GPIO_PORT, BUTTON_ROW4_GPIO_PIN);
}

 

 

	
	

