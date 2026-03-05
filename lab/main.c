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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdint.h"
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
TIM_HandleTypeDef htim6;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
const uint8_t TABLA7SEG[16] =
{
	0x7E, 0x30, 0x6D, 0x79, 0x33, 0x5B, 0x5F, 0x70, 0x7F, 0x7B, 0x77, 0x7F, 0x4E, 0x7E, 0x4F, 0x47
};

volatile uint8_t contador_j1 = 0;
volatile uint8_t contador_j2 = 0;
volatile uint8_t ganador = 0;
volatile uint8_t conteo = 5;

uint16_t disp_show = 0;
uint8_t estado = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM6_Init(void);
/* USER CODE BEGIN PFP */
void j1_leds(uint8_t conteo);
void j2_leds(uint8_t conteo);
void display_set(uint8_t numero);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_USART2_UART_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim6);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  j1_leds(contador_j1);
	  j2_leds(contador_j2);

	  if (estado == 0)
	  {
		  display_set(ganador);
		  if(ganador == 1)
		  {
			  HAL_GPIO_WritePin(GPIOC, J1_ledA_Pin, 1);
			  HAL_GPIO_WritePin(GPIOC, J1_ledB_Pin, 1);
			  HAL_GPIO_WritePin(GPIOC, J1_ledC_Pin, 1);
			  HAL_GPIO_WritePin(GPIOC, J1_ledD_Pin, 1);
		  }
		  else if (ganador == 2)
		  {
			  HAL_GPIO_WritePin(GPIOC, J2_ledA_Pin, 1);
			  HAL_GPIO_WritePin(GPIOC, J2_ledB_Pin, 1);
			  HAL_GPIO_WritePin(GPIOC, J2_ledC_Pin, 1);
			  HAL_GPIO_WritePin(GPIOC, J2_ledD_Pin, 1);
		  }
	  }
	  else if (estado ==  1)
	  {
		  display_set(conteo);
	  }
	  else
	  {
		  display_set(0);
	  }

	  HAL_Delay(100);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 64;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV8;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 16000-1;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 999-1;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, J1_ledA_Pin|J1_ledB_Pin|J1_ledC_Pin|J1_ledD_Pin
                          |J2_ledA_Pin|J2_ledB_Pin|J2_ledC_Pin|J2_ledD_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|Display0_Pin|Display1_Pin|Display2_Pin
                          |Display3_Pin|Display4_Pin|Display5_Pin|Display6_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : J1_ledA_Pin J1_ledB_Pin J1_ledC_Pin J1_ledD_Pin
                           J2_ledA_Pin J2_ledB_Pin J2_ledC_Pin J2_ledD_Pin */
  GPIO_InitStruct.Pin = J1_ledA_Pin|J1_ledB_Pin|J1_ledC_Pin|J1_ledD_Pin
                          |J2_ledA_Pin|J2_ledB_Pin|J2_ledC_Pin|J2_ledD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin Display0_Pin Display1_Pin Display2_Pin
                           Display3_Pin Display4_Pin Display5_Pin Display6_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|Display0_Pin|Display1_Pin|Display2_Pin
                          |Display3_Pin|Display4_Pin|Display5_Pin|Display6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN_1_Pin BTN_2_Pin */
  GPIO_InitStruct.Pin = BTN_1_Pin|BTN_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : START_Pin */
  GPIO_InitStruct.Pin = START_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(START_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == BTN_1_Pin)
	{
		if (estado == 2)
		{
			contador_j1 ++;
			if(contador_j1 == 5)
			{
				contador_j1 = 0;
				contador_j2 = 0;
				ganador = 1;
				estado = 0;
			}
		}
	}
	if(GPIO_Pin == BTN_2_Pin)
	{
		if (estado == 2)
		{
			contador_j2 ++;
			if(contador_j2 == 5)
			{
				contador_j1 = 0;
				contador_j2 = 0;
				ganador = 2;
				estado = 0;
			}
		}
	}
	if(GPIO_Pin == START_Pin)
	{
		if (estado == 0)
		{
			contador_j1 = 0;
			contador_j2 = 0;
			estado = 1;
		}
	}
}

void j1_leds(uint8_t conteo)
{
	if(conteo == 0)
	{
		 HAL_GPIO_WritePin(GPIOC, J1_ledA_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J1_ledB_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J1_ledC_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J1_ledD_Pin, 0);
	}
	else if(conteo == 1)
	{
		 HAL_GPIO_WritePin(GPIOC, J1_ledA_Pin, 1);
		 HAL_GPIO_WritePin(GPIOC, J1_ledB_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J1_ledC_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J1_ledD_Pin, 0);
	}
	else if(conteo == 2)
	{
		 HAL_GPIO_WritePin(GPIOC, J1_ledA_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J1_ledB_Pin, 1);
		 HAL_GPIO_WritePin(GPIOC, J1_ledC_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J1_ledD_Pin, 0);
	}
	else if(conteo == 3)
	{
		 HAL_GPIO_WritePin(GPIOC, J1_ledA_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J1_ledB_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J1_ledC_Pin, 1);
		 HAL_GPIO_WritePin(GPIOC, J1_ledD_Pin, 0);
	}
	else if(conteo == 4)
	{
		 HAL_GPIO_WritePin(GPIOC, J1_ledA_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J1_ledB_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J1_ledC_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J1_ledD_Pin, 1);
	}
}

void j2_leds(uint8_t conteo)
{
	if(conteo == 0)
	{
		 HAL_GPIO_WritePin(GPIOC, J2_ledA_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J2_ledB_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J2_ledC_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J2_ledD_Pin, 0);
	}
	else if(conteo == 1)
	{
		 HAL_GPIO_WritePin(GPIOC, J2_ledA_Pin, 1);
		 HAL_GPIO_WritePin(GPIOC, J2_ledB_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J2_ledC_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J2_ledD_Pin, 0);
	}
	else if(conteo == 2)
	{
		 HAL_GPIO_WritePin(GPIOC, J2_ledA_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J2_ledB_Pin, 1);
		 HAL_GPIO_WritePin(GPIOC, J2_ledC_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J2_ledD_Pin, 0);
	}
	else if(conteo == 3)
	{
		 HAL_GPIO_WritePin(GPIOC, J2_ledA_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J2_ledB_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J2_ledC_Pin, 1);
		 HAL_GPIO_WritePin(GPIOC, J2_ledD_Pin, 0);
	}
	else if(conteo == 4)
	{
		 HAL_GPIO_WritePin(GPIOC, J2_ledA_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J2_ledB_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J2_ledC_Pin, 0);
		 HAL_GPIO_WritePin(GPIOC, J2_ledD_Pin, 1);
	}
}

void display_set(uint8_t numero)
{
	disp_show = TABLA7SEG[numero];
	disp_show = (disp_show << 6);
	uint16_t val_actual = GPIOA->ODR;
	GPIOA->ODR = (val_actual & 0xE03F) | disp_show;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim == &htim6)
	{
		if (estado == 1)
		{
			if (conteo == 0)
			{
				conteo = 6;
				estado = 2;
			}
			conteo --;
		}
		HAL_TIM_Base_Start_IT(&htim6);
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
