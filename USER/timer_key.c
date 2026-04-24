#include "timer_key.h"
#include "button4_4.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_exti.h"
#include "misc.h"

// 去抖参数
#define DEBOUNCE_TICKS 3 // 3 * tick_ms
#define TICK_FREQ_HZ 100 // 100 Hz => 10 ms tick

static volatile int key_event = 0; // 被主循环读取并清零

// 内部状态
static uint8_t last_raw = 0;
static uint8_t stable = 0;
static uint8_t cnt = 0;

// 读取矩阵键的原始按键（不做等待释放）
// 返回 0..16，0 表示无按下
static int read_key_raw(void)
{
    uint8_t col_bits = 0x0f;
    // Row1
    GPIO_ResetBits(BUTTON_ROW1_GPIO_PORT, BUTTON_ROW1_GPIO_PIN);
    col_bits = 0;
    col_bits |= (GPIO_ReadInputDataBit(BUTTON_COL1_GPIO_PORT, BUTTON_COL1_GPIO_PIN) << 3);
    col_bits |= (GPIO_ReadInputDataBit(BUTTON_COL2_GPIO_PORT, BUTTON_COL2_GPIO_PIN) << 2);
    col_bits |= (GPIO_ReadInputDataBit(BUTTON_COL3_GPIO_PORT, BUTTON_COL3_GPIO_PIN) << 1);
    col_bits |= (GPIO_ReadInputDataBit(BUTTON_COL4_GPIO_PORT, BUTTON_COL4_GPIO_PIN));
    GPIO_SetBits(BUTTON_ROW1_GPIO_PORT, BUTTON_ROW1_GPIO_PIN);
    if (col_bits != 0x0f) {
        // map to 1..4
        switch (col_bits) {
            case 0x07: return 1;
            case 0x0b: return 2;
            case 0x0d: return 3;
            case 0x0e: return 4;
            default: break;
        }
    }
    // Row2
    GPIO_ResetBits(BUTTON_ROW2_GPIO_PORT, BUTTON_ROW2_GPIO_PIN);
    col_bits = 0;
    col_bits |= (GPIO_ReadInputDataBit(BUTTON_COL1_GPIO_PORT, BUTTON_COL1_GPIO_PIN) << 3);
    col_bits |= (GPIO_ReadInputDataBit(BUTTON_COL2_GPIO_PORT, BUTTON_COL2_GPIO_PIN) << 2);
    col_bits |= (GPIO_ReadInputDataBit(BUTTON_COL3_GPIO_PORT, BUTTON_COL3_GPIO_PIN) << 1);
    col_bits |= (GPIO_ReadInputDataBit(BUTTON_COL4_GPIO_PORT, BUTTON_COL4_GPIO_PIN));
    GPIO_SetBits(BUTTON_ROW2_GPIO_PORT, BUTTON_ROW2_GPIO_PIN);
    if (col_bits != 0x0f) {
        switch (col_bits) {
            case 0x07: return 5;
            case 0x0b: return 6;
            case 0x0d: return 7;
            case 0x0e: return 8;
            default: break;
        }
    }
    // Row3
    GPIO_ResetBits(BUTTON_ROW3_GPIO_PORT, BUTTON_ROW3_GPIO_PIN);
    col_bits = 0;
    col_bits |= (GPIO_ReadInputDataBit(BUTTON_COL1_GPIO_PORT, BUTTON_COL1_GPIO_PIN) << 3);
    col_bits |= (GPIO_ReadInputDataBit(BUTTON_COL2_GPIO_PORT, BUTTON_COL2_GPIO_PIN) << 2);
    col_bits |= (GPIO_ReadInputDataBit(BUTTON_COL3_GPIO_PORT, BUTTON_COL3_GPIO_PIN) << 1);
    col_bits |= (GPIO_ReadInputDataBit(BUTTON_COL4_GPIO_PORT, BUTTON_COL4_GPIO_PIN));
    GPIO_SetBits(BUTTON_ROW3_GPIO_PORT, BUTTON_ROW3_GPIO_PIN);
    if (col_bits != 0x0f) {
        switch (col_bits) {
            case 0x07: return 9;
            case 0x0b: return 10;
            case 0x0d: return 11;
            case 0x0e: return 12;
            default: break;
        }
    }
    // Row4
    GPIO_ResetBits(BUTTON_ROW4_GPIO_PORT, BUTTON_ROW4_GPIO_PIN);
    col_bits = 0;
    col_bits |= (GPIO_ReadInputDataBit(BUTTON_COL1_GPIO_PORT, BUTTON_COL1_GPIO_PIN) << 3);
    col_bits |= (GPIO_ReadInputDataBit(BUTTON_COL2_GPIO_PORT, BUTTON_COL2_GPIO_PIN) << 2);
    col_bits |= (GPIO_ReadInputDataBit(BUTTON_COL3_GPIO_PORT, BUTTON_COL3_GPIO_PIN) << 1);
    col_bits |= (GPIO_ReadInputDataBit(BUTTON_COL4_GPIO_PORT, BUTTON_COL4_GPIO_PIN));
    GPIO_SetBits(BUTTON_ROW4_GPIO_PORT, BUTTON_ROW4_GPIO_PIN);
    if (col_bits != 0x0f) {
        switch (col_bits) {
            case 0x07: return 13;
            case 0x0b: return 14;
            case 0x0d: return 15;
            case 0x0e: return 16;
            default: break;
        }
    }
    return 0;
}

// 每个 tick 调用一次的去抖处理（在 TIM2 中断中调用）
static void tick_handler(void)
{
    int raw = read_key_raw(); // 0..16

    if (raw == last_raw) {
        if (cnt < 255) cnt++;
        if (cnt >= DEBOUNCE_TICKS) {
            if (stable != last_raw) {
                // 稳定态发生变化
                stable = last_raw;
                if (stable != 0) {
                    // 只有按下时产生事件（避免长按重复）
                    key_event = stable;
                }
            }
        }
    } else {
        // 新的读值，重置计数
        last_raw = raw;
        cnt = 1;
    }
}

int TimerKey_GetKey(void)
{
    int k = key_event;
    if (k != 0) key_event = 0;
    return k;
}

void TimerKey_Init(void)
{
    // TIM2 定时器用于按键去抖扫描
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    // 计算分频器，使定时器计数频率为 TICK_FREQ_HZ * 100 （方便 ARR=99）
    uint32_t psc = SystemCoreClock / (TICK_FREQ_HZ * 100) - 1;
    if (psc > 0xFFFF) psc = 0xFFFF;

    TIM_TimeBaseStructure.TIM_Period = 99; // ARR
    TIM_TimeBaseStructure.TIM_Prescaler = psc;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM2, ENABLE);
}

// TIM2 中断处理函数
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        tick_handler();
    }
}
