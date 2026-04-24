#ifndef _LCD1602_I2C_TEST_H_
#define _LCD1602_I2C_TEST_H_

#include "stm32f1xx_hal.h"

void vLcd1602I2cTestInit(I2C_HandleTypeDef *pstI2cHandle);
void vLcd1602I2cTestRequestHelloWorld(void);
void vLcd1602I2cTestService(void);
void vLcd1602I2cTestShowHelloWorld(I2C_HandleTypeDef *pstI2cHandle);

#endif
