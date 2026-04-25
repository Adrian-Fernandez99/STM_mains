/* USER CODE BEGIN Header */
/**
 * Brittany Herdez
 * Adrian Fernandez
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

/* ===========================================================================
 * ARCHIVO: main.c  —  Módulo de Audio (STM32 NUCLEO-F446RE)
 * ---------------------------------------------------------------------------
 * Qué hace este archivo:
 *   - Genera sonido usando el DAC interno del STM32 (pin PA4)
 *   - Las canciones están guardadas como arreglos de notas en archivos .h
 *   - Recibe comandos por UART para cambiar de canción en tiempo real
 *   - Cada nota se toca generando una onda cuadrada a la frecuencia indicada
 *
 * Canciones disponibles:
 *   1 -> Lapiz     (mini-juego Trace Race)
 *   2 -> Winner    (pantalla de ganador)
 *   3 -> Tronco    (mini-juego Cortar el Tronco)
 *   4 -> Plataformas (mini-juego Mecha Marathon)
 *   5 -> Pepino    (menú principal)
 *   0 -> Detener
 * ===========================================================================
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
/* Notas musicales (frecuencias en Hz) */
#include "pitches.h"
#include <stdio.h>

/* Arreglos de notas y duraciones para cada canción */
#include "pepino.h"
#include "plataformas.h"
#include "tronco.h"
#include "winner.h"
#include "lapiz.h"
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
DAC_HandleTypeDef hdac;    /* Maneja el DAC (convertidor digital a analógico) */
TIM_HandleTypeDef htim1;   /* Timer 1 — configurado pero no se usa directamente */
UART_HandleTypeDef huart2; /* UART para recibir comandos y mandar mensajes de debug */

/* USER CODE BEGIN PV */

/* Canción que se está tocando ahora ('0' = ninguna) */
volatile uint8_t current_option = '0';

/* ---------------------------------------------------------------------------
 * printMenu()
 * Manda el menú de opciones por UART para que el usuario pueda elegir canción.
 * Se usa con un monitor serial (ej. PuTTY o el monitor de STM32CubeIDE).
 * ---------------------------------------------------------------------------*/
void printMenu() {
    char msg[] = "\r\n--- MENU DE AUDIO ---\r\n"
                 "1. Lapiz\r\n"
                 "2. Winner\r\n"
                 "3. Tronco\r\n"
                 "4. Plataformas\r\n"
                 "5. Pepino\r\n"
                 "0. Detener\r\n"
                 "Seleccione opcion:\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, HAL_MAX_DELAY);
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_DAC_Init(void);

/* USER CODE BEGIN PFP */
void playDAC_note(int freq, int duration);
uint8_t playDAC(int *melody, int *duration, int size);
/* USER CODE END PFP */

/* ===========================================================================
 * SECCIÓN: FUNCIONES DE AUDIO
 * ===========================================================================*/
/* USER CODE BEGIN 0 */

/* Frecuencia del timer en Hz (84 MHz para APB2 del F446RE) */
#define TIM_FREQ 84000000
#define ARR 100

/* ---------------------------------------------------------------------------
 * delay_us()
 * Delay preciso en microsegundos usando el contador de ciclos DWT del Cortex-M4.
 * Se usa en lugar de HAL_Delay porque HAL_Delay solo llega a milisegundos,
 * y para generar frecuencias de audio necesitamos microsegundos.
 *
 * Parámetro: us — cuántos microsegundos esperar
 * ---------------------------------------------------------------------------*/
void delay_us(uint32_t us) {
    uint32_t ticks = us * (HAL_RCC_GetHCLKFreq() / 1000000);
    uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < ticks);
}

/* ---------------------------------------------------------------------------
 * playDAC_note()
 * Toca una sola nota usando el DAC.
 *
 * Cómo funciona:
 *   Para generar una frecuencia, pone el DAC en alto (3000) por medio período,
 *   luego en cero por el otro medio período. Eso crea una onda cuadrada.
 *   Por ejemplo, para 440 Hz: período = 2272 µs, cada mitad = 1136 µs.
 *
 * Parámetros:
 *   freq     — frecuencia en Hz (ej. 440 para La4). 0 = silencio.
 *   duration — duración en milisegundos
 *
 * Notas fuera de rango (< 100 Hz o > 2000 Hz) se tratan como silencio
 * porque el buzzer no las reproduce bien.
 * ---------------------------------------------------------------------------*/
void playDAC_note(int freq, int duration) {
    /* Si la frecuencia es 0, es una pausa — solo esperar */
    if (freq == 0) {
        HAL_Delay(duration);
        return;
    }

    /* Filtrar frecuencias fuera del rango util del DAC con delay_us */
    if (freq < 100 || freq > 2000) {
        HAL_Delay(duration);  /* nota fuera de rango = silencio, no ruido */
        return;
    }

    /* Calcular período en microsegundos y cuántos ciclos entran en la duración */
    uint32_t period_us = 1000000 / freq;
    int cycles = (duration * 1000) / period_us;

    /* Generar la onda cuadrada: alto por la mitad del período, bajo por la otra mitad */
    for (int i = 0; i < cycles; i++) {
        HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 3000); /* DAC alto */
        delay_us(period_us / 2);
        HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0);    /* DAC bajo */
        delay_us(period_us / 2);
    }
}

/* ---------------------------------------------------------------------------
 * playDAC()
 * Toca una canción completa nota por nota.
 *
 * Después de cada nota revisa si llegó un nuevo comando por UART.
 * Si llegó uno, para la canción y lo devuelve para que el main cambie de canción.
 * Si terminó toda la canción sin interrupción, devuelve 0 (repetir la misma).
 *
 * Parámetros:
 *   melody   — arreglo con las frecuencias de cada nota
 *   duration — arreglo con las duraciones en ms de cada nota
 *   size     — cantidad de notas en el arreglo
 *
 * Retorna: el nuevo comando recibido, o 0 si terminó normalmente
 * ---------------------------------------------------------------------------*/
uint8_t playDAC(int *melody, int *duration, int size) {
    uint8_t new_option = 0;
    for (int i = 0; i < size; i++) {
        /* Tocar la nota actual */
        playDAC_note(melody[i], duration[i]);

        /* Silencio breve entre notas para separarlas */
        HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0);
        HAL_Delay(30);

        /* Checar si llegó un nuevo comando por UART (sin bloquear) */
        if (HAL_UART_Receive(&huart2, &new_option, 1, 1) == HAL_OK) {
            HAL_UART_Transmit(&huart2, &new_option, 1, HAL_MAX_DELAY);
            return new_option;  /* interrumpir y devolver el nuevo número */
        }
    }
    return 0;  /* terminó normalmente, repetir la misma canción */
}
/* USER CODE END 0 */

/* ===========================================================================
 * SECCIÓN: MAIN — Punto de entrada del programa
 * ===========================================================================*/
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* Inicialización de HAL y periféricos generados por CubeMX ---------------*/
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_USART2_UART_Init();
  MX_DAC_Init();

  /* USER CODE BEGIN 2 */

  /* Arrancar el DAC en el canal 1 (pin PA4) */
  HAL_DAC_Start(&hdac, DAC_CHANNEL_1);

  /* Activar el contador de ciclos DWT — necesario para que funcione delay_us() */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  /* USER CODE END 2 */

  /* ===========================================================================
   * SECCIÓN: LOOP PRINCIPAL — Menú y reproducción de canciones
   * ===========================================================================*/
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      /* Asegurarse de que el DAC esté en silencio antes de mostrar el menú */
      uint8_t option;
      HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0);

      /* Mostrar menú y esperar a que el usuario elija una opción por UART */
      printMenu();
      HAL_UART_Receive(&huart2, &option, 1, HAL_MAX_DELAY);
      HAL_UART_Transmit(&huart2, &option, 1, HAL_MAX_DELAY); /* eco del carácter recibido */
      current_option = option;

      /* Reproducir la canción elegida en loop hasta que el usuario ponga '0' */
      while (current_option != '0') {
          uint8_t interrupted = 0;

          /* Según la opción, tocar la canción correspondiente */
          if (current_option == '1') {
              char msg[] = "\r\nReproduciendo Mario...\r\n";
              HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, HAL_MAX_DELAY);
              interrupted = playDAC(melody1, durations1, melody_size);
          }
          else if (current_option == '2') {
              char msg[] = "\r\nReproduciendo Winner...\r\n";
              HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, HAL_MAX_DELAY);
              interrupted = playDAC(melody_winner, durations_winner, melody_size_winner);
          }
          else if (current_option == '3') {
              char msg[] = "\r\nReproduciendo Tronco...\r\n";
              HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, HAL_MAX_DELAY);
              interrupted = playDAC(melody_tronco, durations_tronco, melody_size_tronco);  /* FIX: antes usaba melody_size_winner */
          }
          else if (current_option == '4') {
              char msg[] = "\r\nReproduciendo Plataformas...\r\n";
              HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, HAL_MAX_DELAY);
              interrupted = playDAC(melody_plataformas, durations_plataformas, melody_size_plataformas);
          }
          else if (current_option == '5') {                                /* FIX: pepino agregado */
              char msg[] = "\r\nReproduciendo Pepino...\r\n";
              HAL_UART_Transmit(&huart2, (uint8_t*)msg, sizeof(msg)-1, HAL_MAX_DELAY);
              interrupted = playDAC(melody_pepino, durations_pepino, melody_size_pepino);
          }
          else {
              break;  /* opción inválida, volver al menú */
          }

          /* Si playDAC recibió un nuevo comando, cambiar a esa canción */
          if (interrupted != 0) {
              current_option = interrupted;
          }
          /* si interrupted == 0, repite la misma canción automáticamente */
      }

  /* USER CODE END WHILE */
  /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/* ===========================================================================
 * SECCIÓN: CONFIGURACIÓN DE RELOJ Y PERIFÉRICOS (generada por CubeMX)
 * ===========================================================================*/

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
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief DAC Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC_Init(void)
{
  /* USER CODE BEGIN DAC_Init 0 */
  /* USER CODE END DAC_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC_Init 1 */
  /* USER CODE END DAC_Init 1 */

  /** DAC Initialization
  */
  hdac.Instance = DAC;
  if (HAL_DAC_Init(&hdac) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT1 config
  */
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  if (HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC_Init 2 */
  /* USER CODE END DAC_Init 2 */
}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{
  /* USER CODE BEGIN TIM1_Init 0 */
  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */
  /* USER CODE END TIM1_Init 1 */

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 50;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */
  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);
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
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
