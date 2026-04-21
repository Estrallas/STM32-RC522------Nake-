/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled_device.h"
#include "pca9685_device.h"
#include "rc522_device.h"
#include "string.h"
#include "stdbool.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
stOledStaticParamTdf stOledInit;
stPca9685StaticParamTdf stPca9685Init;
stRc522StaticParamTdf stRc522Init;
static volatile uint8_t ucPca9685Ready = FALSE;
static volatile uint8_t ucOledRefreshReq = FALSE;
static bool Nfc_Judge = FALSE;
static char acCardUidHexIsr[16] = "-- -- -- --";
static char acCardFlag[] = "F";
static uint32_t ulUidKey;
char acCardUidHex[16] = "-- -- -- --";

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static uint16_t s_usGetOledI2cAddr(void)
{
  if(HAL_I2C_IsDeviceReady(&hi2c1, 0x78, 2u, 20u) == HAL_OK)
  {
    return 0x78;
  }

  if(HAL_I2C_IsDeviceReady(&hi2c1, 0x7A, 2u, 20u) == HAL_OK)
  {
    return 0x7A;
  }

  return 0x78;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  stOledInit.pstI2cHandle = &hi2c1;
  stOledInit.usDevAddr = s_usGetOledI2cAddr();
  vOledDeviceInit(&stOledInit, OLED);


  vUiWriteStringToBuffer(0, 0, (const uint8_t *)"RC522 UID:", emOledFontSize_8x16, emOledPixelShowMode_Positive, OLED);
  vUiWriteStringToBuffer(0, 16, (const uint8_t *)acCardUidHex, emOledFontSize_8x16, emOledPixelShowMode_Positive, OLED);
  vUiWriteStringToBuffer(0, 32, (const uint8_t *)"Card Flag: ", emOledFontSize_6x12, emOledPixelShowMode_Positive, OLED);
    vUiWriteStringToBuffer(64, 32, (const uint8_t *)acCardFlag, emOledFontSize_6x12, emOledPixelShowMode_Positive, OLED);
  vOledRefreshFromBuffer(OLED);


  if(HAL_I2C_IsDeviceReady(&hi2c1, 0x80, 2u, 20u) == HAL_OK)
  {
    stPca9685Init.pstI2cHandle = &hi2c1;
    stPca9685Init.usDevAddr = 0x80;
    stPca9685Init.usServoMinPulseUs = PCA9685_SERVO_MIN_PULSE_US;
    stPca9685Init.usServoMaxPulseUs = PCA9685_SERVO_MAX_PULSE_US;
    vPca9685DeviceInit(&stPca9685Init, PCA9685);
    ucPca9685Ready = TRUE;
  }

  stRc522Init.pstSpiHandle = &hspi1;
  stRc522Init.pstNssPort = RC522_SDA_GPIO_Port;
  stRc522Init.usNssPin = RC522_SDA_Pin;
  stRc522Init.pstRstPort = RC522_RST_GPIO_Port;
  stRc522Init.usRstPin = RC522_RST_Pin;
  vRc522DeviceInit(&stRc522Init, RC522);

  if(HAL_TIM_Base_Start_IT(&htim1) != HAL_OK)
  {
    Error_Handler();
  }

  if(HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
  {
    Error_Handler();
  }

  ucOledRefreshReq = TRUE;



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if(ucOledRefreshReq != FALSE)
    {
      __disable_irq();
      memcpy(acCardUidHex, acCardUidHexIsr, sizeof(acCardUidHex));
      ucOledRefreshReq = FALSE;
      __enable_irq();

      vUiWriteStringToBuffer(0, 16, (const uint8_t *)acCardUidHex, emOledFontSize_8x16, emOledPixelShowMode_Positive, OLED);
      vUiWriteStringToBuffer(64, 32, (const uint8_t *)acCardFlag, emOledFontSize_6x12, emOledPixelShowMode_Positive, OLED);
      vOledRefreshFromBuffer(OLED);
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

  if(htim->Instance == TIM2)
  {

    if(Nfc_Judge==true)
    {
        vPca9685SetServoAngle(Servo_Open, 0, PCA9685);
    }

    else
    {
        vPca9685SetServoAngle(Servo_Close, 0, PCA9685);
    }


  }

  

  else if(htim->Instance == TIM1)
  {
    char acUidTmp[16] = {0};

    ucRc522ReadUidHex(acUidTmp, sizeof(acUidTmp), RC522);
    if(ucRc522UidHexToKey32(acUidTmp, &ulUidKey) != FALSE)
    {
        switch (ulUidKey)
        {
          case 0xDD9AF006:
            Nfc_Judge = TRUE;
            strcpy(acCardFlag, "T");
            break;

          default:
            Nfc_Judge=FALSE;
            strcpy(acCardFlag, "F");
          for(int i = 0; i < PCA9685_CHANNEL_NUM; i++)
          {
              vPca9685SetServoAngle(Servo_Close, i, PCA9685);
          }
            break;
       }
    }
    

    if(memcmp(acCardUidHexIsr, acUidTmp, sizeof(acCardUidHexIsr)) != 0)
    {
      memcpy(acCardUidHexIsr, acUidTmp, sizeof(acCardUidHexIsr));
      ucOledRefreshReq = TRUE;
    }
  }

}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
