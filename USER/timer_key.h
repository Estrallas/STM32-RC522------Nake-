#ifndef __TIMER_KEY_H
#define __TIMER_KEY_H

#include "stm32f10x.h"

void TimerKey_Init(void);
// 获取去抖后的按键事件，返回 0 表示无事件；取到后事件被清除
int TimerKey_GetKey(void);

#endif
