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
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>

#include "ili9341.h"
#include "bitmaps.h"
#include "fatfs_sd.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define min_x 0			// Variables de pantalla
#define min_y 0
#define max_x 320
#define max_y 240

#define transparente 0xf81f // color de transparencia

#define MAX_BUFFER_SIZE 10080 // tamaño maximo de los buffers

FATFS fs;
FATFS *pfs;
FIL fil;
FRESULT fres;
DWORD free_clust;
uint32_t totalSpace, freeSpace;
char buffer[100];
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

static uint16_t buffer_p1[MAX_BUFFER_SIZE];  // buffers personajes
static uint16_t buffer_p2[MAX_BUFFER_SIZE];

static uint16_t erase_p1[MAX_BUFFER_SIZE];		// buffers objetos
static uint16_t erase_p2[MAX_BUFFER_SIZE];

//static uint16_t extra[MAX_BUFFER_SIZE];

static uint16_t buffer_dibujo_SD[10240]; // buffer para fondos

extern const uint16_t fondo[];

uint8_t continiun = 0;

volatile uint16_t pos_p1x = 0; // posisiones del jugador 1
volatile uint16_t pos_p1y = 0;
volatile uint16_t direc_p1 = 0;

volatile uint16_t pos_p2x = 0;  // posisiones jugador 2
volatile uint16_t pos_p2y = 0;
volatile uint16_t direc_p2 = 0;

uint8_t nano_rx; // buffer para leer y enviar datos por uart

volatile uint8_t menu_state = 0;
volatile uint8_t state = 0;  // 0 = Menu - 1 = jugando
volatile uint8_t current_game = 0;

volatile uint8_t p1_ch_select = 0;
volatile uint8_t p2_ch_select = 0;

uint8_t p1_ch = 0;
uint8_t p2_ch = 0;

volatile uint8_t draw_ok = 0;

volatile uint8_t p_press = 0;
volatile uint8_t p1_input;
volatile uint8_t rp_p1_chsel;

volatile uint8_t p2_input;
volatile uint8_t rp_p2_chsel;

uint8_t prev_input1 = 0;
uint8_t prev_input2 = 0;

volatile uint8_t game_select = 0;
volatile uint8_t rp_game_select = 0;
volatile uint8_t game_toplay = 0;

volatile uint8_t minijuego = 0;
volatile uint8_t prioridad = 0;

uint8_t segundos = 0;

uint8_t contador = 0;
uint8_t contador1_4 = 0;

char buffer_contador[10];
char buffer_contador1_4[10];

uint8_t p1_BAscore = 0;
uint8_t p2_BAscore = 0;

uint8_t pepino1 = 150;
uint8_t pepino2 = 150;

uint8_t movicanche = 1;

uint8_t p1_linea = 0;
uint8_t p2_linea = 0;

uint8_t juancarlos = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM6_Init(void);
static void MX_TIM7_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void SPI_SetSpeed(uint32_t prescaler) {
    // 1. Apagar el SPI temporalmente
    __HAL_SPI_DISABLE(&hspi1);

    // 2. Limpiar los bits actuales de velocidad (BR[2:0])
    hspi1.Instance->CR1 &= ~(SPI_CR1_BR_Msk);

    // 3. Escribir el nuevo prescaler
    hspi1.Instance->CR1 |= prescaler;

    // 4. Volver a encender el SPI
    __HAL_SPI_ENABLE(&hspi1);
}

void transmit_uart(char *string) {
	uint8_t len = strlen(string);
	HAL_UART_Transmit(&huart2, (uint8_t*) string, len, 200);
}

void cargar_archivo(char* nombre_archivo, uint16_t* buffer_destino, uint32_t tamano_buffer) {
    UINT bytesRead;

    // 1. APAGAR EL TFT Y ENCENDER LA SD ANTES DE ABRIR EL ARCHIVO
	HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);   // TFT OFF
	HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET); // SD ON

    fres = f_open(&fil, nombre_archivo, FA_READ);

    if (fres != FR_OK) {
    	// Si falla, apagar la SD antes de salir
		HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
		transmit_uart("Error al abrir el archivo .bin\n");
		return;
    }

    // 3. LEER EL ARCHIVO DIRECTO AL BUFFER QUE TÚ ELIJAS
	// Aquí usamos los parámetros que le mandaste a la función
	fres = f_read(&fil, (uint8_t*)buffer_destino, tamano_buffer, &bytesRead);

	// 4. CERRAR ARCHIVO Y APAGAR SD
	f_close(&fil);
	HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET); // SD OFF

	// 5. VALIDACIÓN Y DIAGNÓSTICO
	if (fres != FR_OK || bytesRead == 0) {
		transmit_uart("Error al leer los datos o el archivo estaba vacio\n");
		return;
	}

    transmit_uart("Imagen cargada en el buffer con exito\n");
}

void dibujar_imagen_raw(char* nombre_archivo, uint16_t x, uint16_t y, uint16_t ancho, uint16_t alto) {
    UINT bytesRead;
    uint16_t buffer_pixel[10240];
    char msg[60];
    uint8_t first = 1;

    // TFT OFF antes de tocar SD
    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SD_CS_GPIO_Port,  SD_CS_Pin,  GPIO_PIN_SET);

    // Abrir archivo
    fres = f_open(&fil, nombre_archivo, FA_READ);
    if (fres != FR_OK) {
        sprintf(msg, "Error f_open: %d\n", fres);
        transmit_uart(msg);
        return;
    }
    transmit_uart("Archivo abierto OK\n");

    while (1) {
        // Leer bloque de SD (TFT OFF mientras FatFS usa el bus)
        HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
        HAL_Delay(1); // <-- Dale un respiro al bus antes de activar la SD
        HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET); // SD ON

        FRESULT r = f_read(&fil, buffer_dibujo_SD, sizeof(buffer_dibujo_SD), &bytesRead);
        HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET); // SD OFF
        HAL_Delay(1); // <-- Retardo antes de activar el LCD
        HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET); // LCD ON
        if (r != FR_OK || bytesRead == 0) {
            sprintf(msg, "f_read fin: r=%d bytes=%u\n", r, (unsigned)bytesRead);
            transmit_uart(msg);
            break;
        }

        // Enviar bloque al TFT (SD OFF)
        HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET); // TFT ON

        if (first) {
            // Primer chunk: configurar ventana
            // SetWindows termina con CS=HIGH, hay que volver a bajarlo
            SetWindows(x, y, x + ancho - 1, y + alto - 1);
            HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
            first = 0;
        } else {
            // Chunks siguientes: Write Memory Continue
            HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET); // DC = CMD
            uint8_t cmd = 0x3C;
            HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
        }

        // Enviar pixeles
        HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET); // DC = DATA
        HAL_SPI_Transmit(&hspi1, (uint8_t*)buffer_dibujo_SD, bytesRead, HAL_MAX_DELAY);

        // TFT OFF al terminar chunk
        HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
    }

    HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SD_CS_GPIO_Port,  SD_CS_Pin,  GPIO_PIN_SET);
    f_close(&fil);
    transmit_uart("Imagen cargada con exito\n");
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
-	HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_FATFS_Init();
  MX_TIM6_Init();
  MX_TIM7_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim6);
  HAL_TIM_Base_Start_IT(&htim7);
  HAL_UART_Receive_IT(&huart1,  &nano_rx, 1);

  	/*
  	FillRect(0, 0, 319, 239, 0xFFFF);
  	FillRect(50, 60, 20, 20, 0xF800);
  	FillRect(70, 60, 20, 20, 0x07E0);
  	FillRect(90, 60, 20, 20, 0x001F);

  	LCD_Bitmap(0, 0, 320, 240, fondo);
  //	FillRect(0, 0, 319, 206, 0x421b);
  //

  	LCD_Print("Hola Mundo", 20, 100, 2, 0x001F, 0xCAB9);

  	//LCD_Sprite(int x, int y, int width, int height, unsigned char bitmap[],int columns, int index, char flip, char offset);
  	//LCD_Sprite(60,100,32,32,pesaSprite,4,3,0,1);

  	//LCD_Bitmap(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char bitmap[]);
  	//LCD_Bitmap(0, 0, 320, 240, fondo);

  	for (int x = 0; x < 319; x++) {
  		LCD_Bitmap(x, 116, 15, 15, tile);
  		LCD_Bitmap(x, 68, 15, 15, tile);
  		LCD_Bitmap(x, 207, 15, 15, tile);
  		LCD_Bitmap(x, 223, 15, 15, tile);
  		x += 15;
  	}
	*/
  	//LCD_Bitmap(100, 100, 28, 31, yosh);

 ////////////////////////////////////////////////////
  	// SD STUFF/////
  	// 1. Desactivar
  	  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
  	  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);

  	  // 2. Reset display
  	  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
  	  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);

  	  LCD_Init();
  	  LCD_Clear(0x00); // NEGRO

  	  // 5. Apagar TFT antes de usar SD
  	  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
  	  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);

  	  // 6. SD
  	  // 6. SD
	  fres = f_mount(&fs, "", 0);
	  if (fres == FR_OK) {
		  transmit_uart("SD montada y lista\n");

	  } else {
		  transmit_uart("Error SD\n");
	  }


  	HAL_Delay(100);

  	///////////////////////////////
  	// LCD stuff
  	SPI_SetSpeed(SPI_BAUDRATEPRESCALER_2);
  	transmit_uart("SPI configurado a Alta Velocidad\n");

	LCD_Clear(0x00);


  	/*FillRect(0, 0, max_x, max_y, 0xf81f);
  	FillRect(0, 0, max_x, max_y, 0x0000);

//  	LCD_Print("Hola Mundo", 20, 100, 2, 0x001F, 0xCAB9);

	LCD_Print("Mario", 20, 50, 2, 0xFFFF, 0x0000);
	LCD_Print("Party", 20, 100, 2, 0xFFFF, 0x0000);
	//LCD_Print(text, x, y, fontSize, color, background);

	LCD_Print("PRESIONA A PARA INICIAR", 20, 150, 1, 0x001F, 0x0000);*/

  	int anim_walk = (pos_p1x/10)%3;
  	int anim_temp = 0;
  	uint16_t capricho = 0;

  	uint8_t p1_ready = 0;
	uint8_t p2_ready = 0;



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
		// ESTADO 0: SISTEMA DE MENÚS
		if (state == 0)
		{
			// Pantalla de Inicio
			if (menu_state == 0)
			{
				if (draw_ok == 0)
				{
					HAL_UART_Transmit(&huart1, "0", 1, 100);		// Se pone la música del menú principal
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_256);		// Se cambia la velocidad para hablar con la SD
					dibujar_imagen_raw("start_bg.bin", 0, 0, max_x, max_y); // Se dibuja el fondo de inicio
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_2);			// Se cambia la velocidad para hablar con la TFT
					draw_ok = 1;
				}

				// Se verifica input para avanzar a la selección de personajes
				if (p1_input == 'A')
				{
					draw_ok = 0;
					menu_state = 1;
					p1_input = 'I';
				}
			}

			// Pantalla de Selección de Personajes
			if (menu_state == 1)
			{
				if (draw_ok == 0)
				{
					p1_input = 'I';
					p2_input = 'I';
					p1_ready = 0;
					p2_ready = 0;

					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_256);		// Se cambia la velocidad para hablar con la SD
					dibujar_imagen_raw("ch_s_bg.bin", 0, 0, max_x, max_y); // Se dibuja el fondo de selección
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_2);			// Se cambia la velocidad para hablar con la TFT

					LCD_Print("CONFIRM", 1, 1, 2, 0xFFFF, 0x0000);
					FillRect(84, 133, 20, 20, 0xf81f);
					FillRect(271, 61, 20, 20, 0xf81f);
					draw_ok = 1;
				}

				// Se verifican inputs de confirmación de personaje J1 y J2
				if (p1_input == 'A')
				{
					p1_ready = 1;
					menu_state = 2;
					draw_ok = 0;
					p1_input = 'I';
				}
				else if (p1_input == 'B')
				{
					draw_ok = 0;
					menu_state = 0;
					p1_input = 'I';
				}

				if (p2_input == 'A')
				{
					p2_ready = 1;
					p2_input = 'I';
				}

				// Se verifica si ambos están listos para avanzar
				if(p1_ready == 0 && p2_ready == 0)
				{
					menu_state = 2;
					draw_ok = 0;
				}
			}

			// Pantalla de Selección de Minijuegos
			if (menu_state == 2)
			{
				if (draw_ok == 0)
				{
					continiun = 0;
					game_toplay = 0;
					p1_input = 'I';
					p2_input = 'I';
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_256);		// Se cambia la velocidad para hablar con la SD
					dibujar_imagen_raw("mp_s_bg.bin", 0, 0, max_x, max_y); // Se dibuja el fondo del menú de minijuegos
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_2);			// Se cambia la velocidad para hablar con la TFT
					draw_ok = 1;
				}

				capricho = capricho + 1;
				if (capricho >= 256) capricho = 255;
				else if (capricho == 255)
				{
					// Se verifica input para mover selector de minijuego (Jugador 1)
					if (p1_input == 'L') {
						game_select = game_select + 1;
						if (game_select == 5) game_select = 0;
						p1_input = 'I';
					}
					else if (p1_input == 'R') {
						if (game_select == 0) game_select = 5;
						game_select = game_select - 1;
						p1_input = 'I';
					}

					// Se dibuja rectángulo borrador del selector
					if (game_select != rp_game_select)
					{
						Rect(129, 60, 63, 54, 0xffff);
						Rect(29, 108, 63, 54, 0xffff);
						Rect(93, 138, 63, 54, 0xffff);
						Rect(168, 138, 63, 54, 0xffff);
						Rect(232, 111, 63, 54, 0xffff);
					}

					rp_game_select = game_select;
					capricho = 0;
				}

				// Se dibuja posición actual del selector de minijuego
				if (game_select == 0) Rect(129, 60, 63, 54, 0xf81f);
				else if (game_select == 1) Rect(29, 108, 63, 54, 0xf81f);
				else if (game_select == 2) Rect(93, 138, 63, 54, 0xf81f);
				else if (game_select == 3) Rect(168, 138, 63, 54, 0xf81f);
				else if (game_select == 4) Rect(232, 111, 63, 54, 0xf81f);

				// Se verifica input para confirmar o cancelar minijuego
				if (p1_input == 'A')
				{
					draw_ok = 0;
					state = 1;
					game_toplay = game_select;
					p1_input = 'I';
				}
				else if (p1_input == 'B')
				{
					draw_ok = 0;
					menu_state = 0;
					p1_input = 'I';
				}
			}
		}

		// ESTADO 1: JUGANDO MINIJUEGOS
		else if (state == 1)
		{
			// Minijuego 0: Área de Pruebas
			if (game_toplay == 0)
			{
				continiun = 1;
				game_toplay = 1;
			}

			// Minijuego 1: Tronco (Mashing de Botón)
			else if (game_toplay == 1)
			{
				if (draw_ok == 0)
				{
					HAL_UART_Transmit(&huart1, "5", 1, 100);		// Se pone la música del minijuego 1
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_256);		// Se cambia la velocidad para hablar con la SD
					cargar_archivo("y_win.bin", buffer_p1, 3640);
					cargar_archivo("h_win.bin", buffer_p2, 4620);
					cargar_archivo("stu_on.bin", erase_p1, 14560);
					cargar_archivo("stu2_on.bin", erase_p2, 14560);
					dibujar_imagen_raw("stump_bg.bin", 0, 0, max_x, max_y); // Se dibuja el fondo del minijuego 1
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_2);			// Se cambia la velocidad para hablar con la TFT
					draw_ok = 1;
				}

				LCD_Print("PRESS BEFORE", 1, 1, 2, 0xFFFF, 0x0000);

				// Se verifica input para iniciar minijuego
				if (p1_input == 'A')
				{
					draw_ok = 0;
					int PB_marker1 = 0;
					int PB_marker2 = 0;
					p1_input = 'I';
					contador = 6;

					LCD_Sprite(60, 90, 26, 35, buffer_p1, 2, 0, 1, 0, 0xf81f, erase_p1, 96, 31, 86, 0);			// Se dibuja sprite actual J1
					LCD_Sprite(120, 87, 33, 35, buffer_p2, 2, 0, 1, 0, 0xf81f, erase_p2, 96, 186, 85, 0);		// Se dibuja sprite actual J2

					// Se inicia animación de cuenta regresiva
					while(contador != 0)
					{
						sprintf(buffer_contador, "%d", contador);
						LCD_Print(buffer_contador, 50, 170, 2, 0xFFFF, 0x0000);
					}

					for (int i = 0; i < 4; i++)
					{
						LCD_Print("ESPERA", 40, 190, 2, 0xFFFF, 0x0000);
						contador = i + 2;
						while(contador != 0){}

						prioridad = 0;
						LCD_Print("PRESSS", 40, 190, 2, 0xFFFF, 0x630c);

						// Se verifican inputs de Mashing de botones J1 y J2
						while (p1_input != 'A' && p2_input != 'A') {}

						// Se inicia animación de golpe ganador
						if (prioridad == 1)
						{
							contador = 5;
							while(contador != 0)
							{
								PB_marker1 ++;
								anim_temp = contador;
								LCD_Print("JGD #1", 40, 190, 2, 0xFFFF, 0x0000);
								LCD_Sprite(60, 90, 26, 35, buffer_p1, 2, 1, 1, 0, 0xf81f, erase_p1, 96, 31, 86, 0); // Se dibuja sprite actual J1
							}
						}
						else if (prioridad == 2)
						{
							contador = 5;
							while(contador != 0)
							{
								PB_marker2 ++;
								anim_temp = contador;
								LCD_Print("JGD #2", 40, 190, 2, 0xFFFF, 0x0000);
								LCD_Sprite(120, 87, 33, 35, buffer_p2, 2, 1, 1, 0, 0xf81f, erase_p2, 96, 186, 85, 0); // Se dibuja sprite actual J2
							}
						}

						p1_input = 'I';
						p2_input = 'I';
					}

					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_256);		// Se cambia la velocidad para hablar con la SD
					cargar_archivo("u_win.bin", buffer_p1, 14560);
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_2);			// Se cambia la velocidad para hablar con la TFT

					HAL_UART_Transmit(&huart1, "6", 1, 100);		// Se pone la música de victoria
					LCD_Sprite(65, 80, 182, 40, buffer_p1, 1, 0, 0, 0, 0xf81f, erase_p1, 96, 186, 85, 0);			// Se dibuja sprite actual de texto ganador

					if (PB_marker1 >= PB_marker2) LCD_Print("Jugador 1", 50, 170, 2, 0xFFFF, 0x0000);
					else LCD_Print("Jugador 2", 50, 170, 2, 0xFFFF, 0x0000);

					p1_input = 'I';
					p2_input = 'I';
					LCD_Print("END OF GAME", 40, 190, 2, 0xFFFF, 0x0000);

					contador = 5;
					while(contador != 0){}

					if (continiun == 1) game_toplay = 2;
					else { state = 0; menu_state = 2; }
				}
			}

			// Minijuego 2: Acantilado (A+B War)
			else if (game_toplay == 2)
			{
				if (draw_ok == 0)
				{
					HAL_UART_Transmit(&huart1, "4", 1, 100);		// Se pone la música del minijuego 2
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_256);		// Se cambia la velocidad para hablar con la SD
					dibujar_imagen_raw("cliff_bg.bin", 0, 0, max_x, max_y); // Se dibuja el fondo del minijuego 2
					cargar_archivo("y_bot.bin", buffer_p1, 4352);
					cargar_archivo("h_btn.bin", buffer_p2, 4488);
					cargar_archivo("cli_on.bin", erase_p1, 15960);
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_2);			// Se cambia la velocidad para hablar con la TFT
					draw_ok = 1;
				}

				LCD_Print("AB WAR", 1, 1, 2, 0xFFFF, 0x0000);

				// Se verifica input para iniciar minijuego
				if (p1_input == 'A')
				{
					draw_ok = 0;
					p1_input = 'I';
					contador = 6;

					// Se inicia animación de cuenta regresiva
					while(contador != 0)
					{
						sprintf(buffer_contador, "%d", contador);
						LCD_Print(buffer_contador, 250, 15, 2, 0xFFFF, 0x0000);
					}
					contador = 10;

					// Se inicia animación de forcejeo entre jugadores
					while(contador != 0)
					{
						contador1_4 = 4;
						juancarlos = 1;
						anim_temp = (contador/4)%2;
						LCD_Sprite(75, 87, 32, 34, buffer_p1, 2, anim_temp, 1, 0, 0xf81f, erase_p1, 76, 63, 68, 0); // Se dibuja sprite actual J1

						anim_temp = (contador/4)%3;
						LCD_Sprite(120, 87, 22, 34, buffer_p2, 3, anim_temp, 1, 0, 0xf81f, erase_p1, 76, 63, 68, 0); // Se dibuja sprite actual J2

						// Se verifican inputs de mashing de botones J1 y J2
						if(p1_input == 'A' || p1_input == 'B') {
							if (prev_input1 != p1_input) p1_BAscore++;
							prev_input1 = p1_input;
						}
						if(p2_input == 'A' || p2_input == 'B') {
							if (prev_input2 != p2_input) p2_BAscore++;
							prev_input2 = p2_input;
						}
					}

					juancarlos = 0;

					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_256);		// Se cambia la velocidad para hablar con la SD
					cargar_archivo("shy_fly.bin", buffer_p1, 12096);
					cargar_archivo("shy_fall.bin", buffer_p2, 3456);
					cargar_archivo("cli2_on.bin", erase_p1, 17980);
					cargar_archivo("fly_on.bin", erase_p2, 19200);
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_2);			// Se cambia la velocidad para hablar con la TFT

					uint8_t shy_mov1 = 0;
					uint8_t shy_mov2 = 0;

					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_256);		// Se cambia la velocidad para hablar con la SD
					dibujar_imagen_raw("fly_bg.bin", 0, 0, max_x, max_y); // Se dibuja fondo de caída libre
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_2);			// Se cambia la velocidad para hablar con la TFT

					contador = 4;

					// Se inicia animación de caída libre y vuelo
					while(contador != 0)
					{
						shy_mov1 = 90 + contador*2;
						shy_mov2 = 90 + contador*2;
						anim_temp = contador;

						LCD_Sprite(110, shy_mov1, 36, 42, buffer_p1, 4, anim_temp, 1, 0, 0xf81f, erase_p2, 80, 100, 80, 0); // Se dibuja sprite actual vuelo
						LCD_Sprite(170, shy_mov2, 36, 42, buffer_p1, 4, anim_temp, 1, 0, 0xf81f, erase_p2, 80, 100, 80, 0); // Se dibuja sprite actual vuelo
					}

					LCD_Sprite(170, shy_mov2, 32, 42, buffer_p2, 1, 0, 1, 0, 0xf81f, erase_p2, 80, 100, 80, 0 );			// Se dibuja sprite actual caída

					contador = 2;
					while(contador != 0) {}

					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_256);		// Se cambia la velocidad para hablar con la SD
					cargar_archivo("u_win.bin", buffer_p1, 14560);
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_2);			// Se cambia la velocidad para hablar con la TFT

					HAL_UART_Transmit(&huart2, "6. Winner\r\n", 1, 100); // Se pone la música de victoria
					LCD_Sprite(65, 80, 182, 40, buffer_p1, 1, 0, 0, 0, 0xf81f, erase_p2, 80, 0, 0, 0);						// Se dibuja sprite actual texto ganador

					if (p1_BAscore >= p2_BAscore) LCD_Print("Jugador 1", 75, 125, 2, 0xFFFF, 0x0000);
					else LCD_Print("Jugador 2", 75, 125, 2, 0xFFFF, 0x0000);

					p1_input = 'I';
					p2_input = 'I';
					LCD_Print("END OF GAME", 75, 200, 2, 0xFFFF, 0x0000);

					contador = 5;
					while(contador != 0){}
					if (continiun == 1) game_toplay = 3;
					else { state = 0; menu_state = 2; }
				}
			}

			// Minijuego 3: Pepino Cutter
			else if (game_toplay == 3)
			{
				if (draw_ok == 0)
				{
					HAL_UART_Transmit(&huart1, "2", 1, 100);		// Se pone la música del minijuego 3
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_256);		// Se cambia la velocidad para hablar con la SD
					dibujar_imagen_raw("kit_bg.bin", 0, 0, max_x, max_y); // Se dibuja fondo de cocina
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_2);			// Se cambia la velocidad para hablar con la TFT
					draw_ok = 1;
					pepino1 = 150;
					pepino2 = 150;
				}

				LCD_Print("PEPINO CUTTER", 1, 1, 2, 0xFFFF, 0x0000);

				// Se verifica input para iniciar minijuego
				if (p1_input == 'A')
				{
					FillRect(0, 0, max_x, max_y, 0xfea0);

					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_256);		// Se cambia la velocidad para hablar con la SD
					cargar_archivo("y_walk.bin", buffer_p1, 6120);
					cargar_archivo("h_walk.bin", buffer_p2, 5760);
					cargar_archivo("pepino.bin", erase_p1, 5440);
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_2);			// Se cambia la velocidad para hablar con la TFT

					LCD_Sprite_simple(230, 50, 85, 32, erase_p1, 1, 0, 0, 0, 0xf81f, 0xfea0);		// Se dibuja sprite actual pepino J1
					LCD_Sprite_simple(230, 158, 85, 32, erase_p1, 1, 0, 0, 0, 0xf81f, 0xfea0);		// Se dibuja sprite actual pepino J2

					draw_ok = 0;
					p1_input = 'I';
					contador = 6;

					// Se inicia animación de cuenta regresiva
					while(contador != 0)
					{
						sprintf(buffer_contador, "%d", contador);
						LCD_Print(buffer_contador, 12, 95, 2, 0xFFFF, 0x0000);
					}

					// Se inicia animación de corte de pepino por turnos
					while(pepino1 != 0 && pepino2 != 0)
					{
						pos_p1x = 285 - pepino1;
						pos_p2x = 285 - pepino2;

						LCD_Sprite_simple(pos_p1x, 50, 30, 34, buffer_p1, 3, (pos_p1x/10)%3, 1, 0, 0xf81f, 0xfea0);		// Se dibuja sprite actual J1 cortando
						LCD_Sprite_simple(pos_p2x, 158, 20, 36, buffer_p2, 4, (pos_p2x/10)%4, 0, 0, 0xf81f, 0xfea0);	// Se dibuja sprite actual J2 cortando

						// Se verifican inputs de corte rápido J1
						if(p1_input == 'R' || p1_input == 'L' || p1_input == 'D' || p1_input == 'U') {
							if (prev_input1 != p1_input) pepino1--;
							prev_input1 = p1_input;
						}
						// Se verifican inputs de corte rápido J2
						if(p2_input == 'R' || p2_input == 'L' || p2_input == 'D' || p2_input == 'U') {
							if (prev_input2 != p2_input) pepino2--;
							prev_input2 = p2_input;
						}
					}

					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_256);		// Se cambia la velocidad para hablar con la SD
					cargar_archivo("y_win.bin", buffer_p1, 3640);
					cargar_archivo("h_win.bin", buffer_p2, 4620);
					cargar_archivo("u_win.bin", erase_p1, 14560);
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_2);			// Se cambia la velocidad para hablar con la TFT

					HAL_UART_Transmit(&huart2, "6", 1, 100);		// Se pone la música de victoria
					LCD_Sprite_simple(10, 70,  182, 40, erase_p1, 1, 0, 0, 0, 0xf81f, 0xfea0);				// Se dibuja sprite actual texto ganador

					// Se inicia animación de ganador del pepino
					if (pepino1 == 0)
					{
						LCD_Print("Jugador 1", 40, 180, 2, 0xFFFF, 0x0000);
						contador = 4;
						while(contador != 0)
						{
							anim_temp = contador/2;
							LCD_Sprite_simple(pos_p1x - 5, 50, 26, 32, buffer_p1, 2, anim_temp, 1, 0, 0xf81f, 0xfea0); // Se dibuja sprite actual celebración J1
						}
					}
					else
					{
						LCD_Print("Jugador 2", 40, 180, 2, 0xFFFF, 0x0000);
						contador = 4;
						while(contador != 0)
						{
							anim_temp = contador/2;
							LCD_Sprite_simple(pos_p2x - 5, 158, 33, 35, buffer_p2, 2, anim_temp, 1, 0, 0xf81f, 0xfea0); // Se dibuja sprite actual celebración J2
						}
					}

					p1_input = 'I';
					p2_input = 'I';
					LCD_Print("END OF GAME", 40, 200, 2, 0xFFFF, 0x0000);

					contador = 5;
					while(contador != 0){}

					if (continiun == 1) game_toplay = 4;
					else { state = 0; menu_state = 2; }
				}
			}

			// Minijuego 4: Follow The Line (Draw)
			else if (game_toplay == 4)
			{
				if (draw_ok == 0)
				{
					HAL_UART_Transmit(&huart1, "1", 1, 100);		// Se pone la música del minijuego 4
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_256);		// Se cambia la velocidad para hablar con la SD
					cargar_archivo("y_walk.bin", buffer_p1, 6120);
					cargar_archivo("h_walk.bin", buffer_p2, 5760);
					cargar_archivo("lapiz.bin", erase_p1, 351);
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_2);			// Se cambia la velocidad para hablar con la TFT

					LCD_Bitmap(0, 0, max_x, max_y, fondo);			// Se dibuja fondo de trazo
					draw_ok = 1;
					pos_p2y = 170;
				}

				LCD_Print("DRAW", 100, 90, 2, 0xFFFF, 0x0000);
				LCD_Print("(u could follow the line)", 90, 120, 1, 0xFFFF, 0x0000);

				// Se verifica input para iniciar minijuego
				if (p1_input == 'A')
				{
					draw_ok = 0;
					p1_input = 'I';

					contador = 6;
					// Se inicia animación de cuenta regresiva
					while(contador != 0)
					{
						sprintf(buffer_contador, "%d", contador);
						LCD_Print(buffer_contador, 12, 95, 2, 0xFFFF, 0x0000);
					}

					int conteo1 = 0;
					int conteo2 = 0;

					pos_p1x = 17;
					pos_p1y = 31;
					pos_p2x = 17;
					pos_p2y = 170;
					movicanche = 1;
					contador = 22;

					// Se inicia animación de avance dibujando la línea
					while(contador != 0)
					{
						LCD_Sprite_unmodified(pos_p1x, pos_p1y, 30, 34, buffer_p1, 3, anim_walk, 1, 0, 0xf81f, fondo);		// Se dibuja sprite actual J1 dibujando
						LCD_Sprite_unmodified(pos_p1x - 9, pos_p1y, 9, 39, erase_p1, 1, 0, 0, 0, 0xf81f, fondo);			// Se dibuja sprite actual lápiz J1

						LCD_Sprite_unmodified(pos_p2x, pos_p2y, 20, 36, buffer_p2, 4, anim_walk, 0, 0, 0xf81f, fondo);		// Se dibuja sprite actual J2 dibujando
						LCD_Sprite_unmodified(pos_p2x - 9, pos_p2y, 9, 39, erase_p1, 1, 0, 0, 0, 0xf81f, fondo);			// Se dibuja sprite actual lápiz J2

						// Se verifica input Jugador 1 para subir y bajar
						if (p1_input == 'D') {
							if (pos_p1y <= 80 - 31) pos_p1y = pos_p1y+2;
							p1_input = 'I';
							anim_walk = (pos_p1y/10)%3;
						}
						else if (p1_input == 'U') {
							if (pos_p1y > min_y+10) pos_p1y = pos_p1y-2;
							p1_input = 'I';
							anim_walk = (pos_p1y/10)%3;
						}

						// Se verifica input Jugador 2 para subir y bajar
						if (p2_input == 'D') {
							if (pos_p2y <= max_y - 40) pos_p2y = pos_p2y+2;
							p2_input = 'I';
							anim_walk = (pos_p2y/10)%3;
						}
						else if (p2_input == 'U') {
							if (pos_p2y > 150) pos_p2y = pos_p2y-2;
							p2_input = 'I';
							anim_walk = (pos_p2y/10)%3;
						}

						// Se verifica acierto de puntos en x predeterminados
						if (pos_p1x == 16 || pos_p1x == 40 || pos_p1x == 64 || pos_p1x == 88 ||
							pos_p1x == 112 || pos_p1x == 136 || pos_p1x == 160 || pos_p1x == 184 ||
							pos_p1x == 208 || pos_p1x == 232 || pos_p1x == 256 || pos_p1x == 280)
						{
						    conteo1 = conteo1 + pos_p1y;
						    conteo2 = conteo2 + pos_p2y;
						}
					}

					movicanche = 0;
					p1_linea = (46.25)-(conteo1/12);
					p2_linea = (190.25)-(conteo2/12);

					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_256);		// Se cambia la velocidad para hablar con la SD
					cargar_archivo("y_win.bin", buffer_p1, 3640);
					cargar_archivo("h_win.bin", buffer_p2, 4620);
					cargar_archivo("u_win.bin", erase_p1, 14560);
					SPI_SetSpeed(SPI_BAUDRATEPRESCALER_2);			// Se cambia la velocidad para hablar con la TFT

					HAL_UART_Transmit(&huart2, "6. Winner\r\n", 1, 100); // Se pone la música de victoria
					LCD_Sprite_unmodified(10, 70,  182, 40, erase_p1, 1, 0, 0, 0, 0xf81f, fondo); // Se dibuja sprite actual texto ganador

					// Se inicia animación de victoria final
					if (p1_linea >= p2_linea)
					{
						LCD_Print("Jugador 1", 40, 180, 2, 0xFFFF, 0x0000);
						contador = 5;
						while(contador != 0)
						{
							anim_temp = contador;
							LCD_Sprite_unmodified(pos_p1x, pos_p1y, 30, 34, buffer_p1, 3, anim_temp, 1, 0, 0xf81f, fondo); // Se dibuja sprite actual celebración J1
							LCD_Print("END OF GAME", 40, 200, 2, 0xFFFF, 0x0000);
						}
					}
					else
					{
						LCD_Print("Jugador 2", 40, 180, 2, 0xFFFF, 0x0000);
						contador = 5;
						while(contador != 0)
						{
							anim_temp = contador;
							LCD_Sprite_unmodified(pos_p2x, pos_p2y, 20, 36, buffer_p2, 4, anim_temp, 0, 0, 0xf81f, fondo); // Se dibuja sprite actual celebración J2
							LCD_Print("END OF GAME", 40, 200, 2, 0xFFFF, 0x0000);
						}
					}

					p1_input = 'I';
					p2_input = 'I';
					state = 0;
					menu_state = 2;
				}
			}
		}
	}

	//V_line( x -1, 100, 30, 0xf81f);

	/*
	for (int var = 0; var < max_y -28;  var++) {
			int anim = (var / 10) % 3;
			LCD_Sprite(150, var, 28, 31, yoshi_walk, 3, anim, 1, 0);
			//V_line(var + 24, 100, 30, 0xf81f);
			HAL_Delay(15);
			}*/

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  /* USER CODE END 3 */


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
  RCC_OscInitStruct.PLL.PLLN = 84;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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
  htim6.Init.Period = 6561-1;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM6_Init 2 */

  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief TIM7 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 16000-1;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 1640-1;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */

  /* USER CODE END TIM7_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_RESET_Pin */
  GPIO_InitStruct.Pin = LCD_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(LCD_RESET_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_DC_Pin */
  GPIO_InitStruct.Pin = LCD_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(LCD_DC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_CS_Pin SD_CS_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin|SD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart == &huart1)
    {
    	//HAL_UART_Transmit(&huart2, (uint8_t*)nano_rx, strlen(left), 1000);
    	HAL_UART_Transmit(&huart2, &nano_rx, 1, 100);
        HAL_UART_Receive_IT(&huart1, &nano_rx, 1);

        if (p_press == 1)
		{
			p1_input = nano_rx;
			p_press = 0;
		}
		else if (p_press == 2)
		{
			p2_input = nano_rx;
			p_press = 0;
		}

        if (nano_rx == '1')
        {
			p_press = 1;
			if (prioridad == 0)
			{
				prioridad = 1;
			}
        }
        else if (nano_rx == '2')
        {
        	p_press = 2;
        	if (prioridad == 0)
			{
				prioridad = 2;
			}
        }


    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim == &htim6)
	{
		if (contador != 0)
		{
			contador--;
		}
		HAL_TIM_Base_Start_IT(&htim6);
	}
	if(htim == &htim7)
	{
		if (movicanche == 1)
		{
			pos_p1x = pos_p1x + 3;
			pos_p2x = pos_p2x + 3;
		}

		if (juancarlos == 1)
		{
			contador1_4 --;

		}
		HAL_TIM_Base_Start_IT(&htim7);
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
