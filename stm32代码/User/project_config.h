/**
  * @file       project_config.h
  * @author     拉咯比哩
  * @version    V1.0.1
  * @date       20260103
  * @brief      工程配置文件
  *
  * <h2><center>&copy;此文件版权归【拉咯比哩】所有.</center></h2>
  */ 
  
#ifndef _PROJECT_CONFIG_H_
#define _PROJECT_CONFIG_H_

#define TRUE                                            1
#define FALSE                                           0

#define STM32_IC_HAL                                    "stm32f1xx_hal.h"

#define LED_DEV_NUM                                     9                               // LED 设备数
#define LED_BOARD                                       emLedDevNum0
#define LED1                                            emLedDevNum1
#define LED2                                            emLedDevNum2
#define LED3                                            emLedDevNum3
#define LED4                                            emLedDevNum4
#define LED5                                            emLedDevNum5
#define LED6                                            emLedDevNum6
#define LED7                                            emLedDevNum7
#define LED8                                            emLedDevNum8

#define DHT11_DEV_NUM                                   1                               // DHT11 设备数
#define DHT11                                           emDht11DevNum0

#define RING_BUFFER_DEV_NUM                             1                               // 环形缓冲区设备数
#define UART_RX_BUFFER                                  emRingBufferDevNum0

#define INTERPRETER_DEV_NUM                             1                               // 命令解释器设备数
#define CMD_INTERPRETER_DILIVERY_NUM_MAX                5                               // 命令解释器分割最大段数
#define CMD_INTERPRETER_DILIVER_STRING                  " "                             // 命令解释器分割字符串

#define OLED_DEV_NUM                                    1                               // OLED 设备数
#define OLED                                            emOledDevNum0
#define OLED_POINT_WIDTH                                128
#define OLED_POINT_HEIGHT                               64
#define OLED_BUFFER_WIDTH                               OLED_POINT_WIDTH                // OLED 一帧数据的行宽度，等于屏幕一行的像素数量
#define OLED_BUFFER_HEIGHT                              OLED_POINT_HEIGHT / 8           // OLED 一帧数据的列高度，等于屏幕一列的像素数量的 1/8

#define PCA9685_DEV_NUM                                 1                               // PCA9685 设备数
#define PCA9685                                         emPca9685DevNum0
#define PCA9685_CHANNEL_NUM                             16                              // PCA9685 PWM 通道数
#define PCA9685_SERVO_ANGLE_MAX                         180.0f                          // 舵机最大角度
#define PCA9685_SERVO_ANGLE_MIN                         0.0f                            // 舵机最小角度
#define PCA9685_SERVO_DEFAULT_FREQ_HZ                   50                              // 舵机默认 PWM 频率
#define PCA9685_SERVO_MIN_PULSE_US                      500                             // 舵机最小脉宽（us）
#define PCA9685_SERVO_MAX_PULSE_US                      2500                            // 舵机最大脉宽（us）

#define RC522_DEV_NUM                                   1                               // RC522 设备数
#define RC522                                           emRc522DevNum0

#endif

