#ifndef __SERVO_H
#define __SERVO_H
 
void PCA9685_Write(uint8_t reg, uint8_t data);
void PCA9685_SetPWMFreq(float freq);
void PCA9685_Init(void);
void PCA9685_SetAngle(uint8_t channel, float angle);
 
#endif
