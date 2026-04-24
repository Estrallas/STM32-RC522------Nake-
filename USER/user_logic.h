#ifndef __USER_LOGIC_H__
#define __USER_LOGIC_H__

#include "stm32f10x.h"

void UserLogic_Init(void);
void UserLogic_HandleKey(int key);
void UserLogic_HandleRFID(u8 *uid);
void UserLogic_Loop(void);

#endif
