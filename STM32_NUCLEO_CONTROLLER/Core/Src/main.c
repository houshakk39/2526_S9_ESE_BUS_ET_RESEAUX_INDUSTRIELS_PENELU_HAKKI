/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
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
#include "bmp280.h"
#include "mpu9250.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
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
CAN_HandleTypeDef hcan1;

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
BMP280_HandleTypedef bmp;
mpu9250_raw_data_t   imu;

/* Buffer de recepção UART1 (pour les commandes venant du RPi) */
#define CMD_BUF_LEN   32

static char    g_cmd_buf[CMD_BUF_LEN];

/* índice e flag acessados por interrupção + main */
static volatile uint8_t g_cmd_idx   = 0;
static volatile uint8_t g_cmd_ready = 0;

/* buffers de recepção para interrupção (1 byte por vez) */
static uint8_t uart1_rx_byte = 0;
static uint8_t uart2_rx_byte = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_CAN1_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* Valeurs globales compensées / calculées */
static volatile int32_t  g_temp_centi  = 0;   // T en 0.01°C
static volatile uint32_t g_press_pa    = 0;   // P en Pa
static volatile int32_t  g_angle_milli = 0;   // angle en 0.001 deg (placeholder)

/* Coefficient K stocké en 1/100 (SET_K=1234 -> K = 12.34) */
static volatile int32_t  g_K_centi     = 100; // valeur par défaut : 1.00

/* Buffer de réception UART1 (pour les commandes venant du RPi) */
#define CMD_BUF_LEN   32
static char    g_cmd_buf[CMD_BUF_LEN];

static void Sensors_Update(void)
{
	uint32_t raw_temp, raw_press;
	BMP280_S32_t T;
	BMP280_U32_t P;

	if (BMP280_ReadRaw(&bmp, &raw_temp, &raw_press) == HAL_OK)
	{
		T = BMP280_Compensate_T_int32(&bmp, (BMP280_S32_t)raw_temp);
		P = BMP280_Compensate_P_int32(&bmp, (BMP280_S32_t)raw_press);

		g_temp_centi = (int32_t)T;
		g_press_pa   = (uint32_t)P;
	}

	/* Lecture du MPU9250 (optionnel pour l'angle) */
	if (mpu9250_read_raw(&imu) == HAL_OK)
	{
		/* TODO: calcul de l’angle à partir de l’accéléro/gyro.
		 * Pour l'instant, on met un placeholder à 0.000 deg.
		 */
		g_angle_milli = 0;
	}
}
static void Protocol_SendString(const char *s)
{
	/* Envoi uniquement sur UART1 vers Raspberry (printf já manda nos dois) */
	HAL_UART_Transmit(&huart1, (uint8_t *)s, strlen(s), HAL_MAX_DELAY);
}

/**
 * @brief  Traite une commande complète reçue sur UART1 (sans \r\n).
 *
 * @param  cmd  Chaîne C terminée par '\0', par exemple "GET_T".
 */
static void Protocol_HandleCommand(const char *cmd)
{
	char tx[32];

	/* Commande GET_T : T=+12.50_C  (format lisible) */
	if (strncmp(cmd, "GET_T", 5) == 0)
	{
		int32_t t = g_temp_centi;
		char sign = '+';
		if (t < 0)
		{
			sign = '-';
			t = -t;
		}
		int32_t t_int  = t / 100;
		int32_t t_frac = t % 100;

		/* Exemple de format : T=+12.34_C */
		snprintf(tx, sizeof(tx), "T=%c%02ld.%02ld_C\r\n",
				sign, (long)t_int, (long)t_frac);
		Protocol_SendString(tx);
	}
	/* Commande GET_P : P=102300Pa */
	else if (strncmp(cmd, "GET_P", 5) == 0)
	{
		uint32_t p = g_press_pa;
		snprintf(tx, sizeof(tx), "P=%luPa\r\n", (unsigned long)p);
		Protocol_SendString(tx);
	}
	/* Commande SET_K=1234 */
	else if (strncmp(cmd, "SET_K=", 6) == 0)
	{
		const char *value_str = cmd + 6;
		int32_t k = (int32_t)atoi(value_str);  // en 1/100

		g_K_centi = k;

		snprintf(tx, sizeof(tx), "SET_K=OK\r\n");
		Protocol_SendString(tx);
	}
	/* Commande GET_K : K=12.34000  (K en 1/100, mais affiché avec 5 décimales) */
	else if (strncmp(cmd, "GET_K", 5) == 0)
	{
		int32_t k = g_K_centi; // ex : 1234 -> 12.34
		int32_t k_int  = k / 100;
		int32_t k_frac = k % 100;
		if (k_frac < 0) k_frac = -k_frac;

		/* Format demandé : K=12.34000 (2 décimales réelles + 3 zéros) */
		snprintf(tx, sizeof(tx), "K=%ld.%02ld000\r\n",
				(long)k_int, (long)k_frac);
		Protocol_SendString(tx);
	}
	/* Commande GET_A : A=125.7000 (angle sur 10 caractères)
	 * Pour le moment, angle stocké en 0.001 deg (milli-deg)
	 */
	else if (strncmp(cmd, "GET_A", 5) == 0)
	{
		int32_t a = g_angle_milli;  // ex: 125700 -> 125.700 deg
		int32_t a_int  = a / 1000;
		int32_t a_frac = a % 1000;
		if (a_frac < 0) a_frac = -a_frac;

		/* Format : A=xxx.yyyy  -> ici : 3 décimales, on complète avec un zéro */
		/* Exemple : 125.7000 */
		snprintf(tx, sizeof(tx), "A=%ld.%03ld0\r\n",
				(long)a_int, (long)a_frac);
		Protocol_SendString(tx);
	}
	/* Commande inconnue */
	else
	{
		snprintf(tx, sizeof(tx), "ERR=CMD\r\n");
		Protocol_SendString(tx);
	}
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
  MX_USART2_UART_Init();
  MX_CAN1_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	/* USER CODE BEGIN 2 */
	printf("\r\n=== Init capteurs ===\r\n");

	if (BMP280_Init(&bmp, &hi2c1, BMP280_I2C_ADDR_DEFAULT) != HAL_OK)
	{
		printf("Erreur init BMP280\r\n");
	}

	if (mpu9250_init() != HAL_OK)
	{
		printf("Erreur init MPU9250\r\n");
	}

	/* Iniciar recepção por interrupção em UART1 (Raspberry) e UART2 (terminal) */
	HAL_UART_Receive_IT(&huart1, &uart1_rx_byte, 1);
	HAL_UART_Receive_IT(&huart2, &uart2_rx_byte, 1);

	printf("=== Protocole UART1 pret (Raspberry Pi) ===\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
		/* Mise à jour périodique des capteurs */
		Sensors_Update();

		/* Se alguma linha de comando foi completada na UART1 (via interrupção) */
		if (g_cmd_ready)
		{
			/* Limpa flag antes de tratar, pra evitar race condition simples */
			g_cmd_ready = 0;

			/* Aqui é seguro chamar printf, HAL_UART_Transmit, etc. */
			Protocol_HandleCommand(g_cmd_buf);
		}

		HAL_Delay(50);  // ~20 Hz
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
  * @brief CAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 16;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_1TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_1TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  /* USER CODE END CAN1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	/* Recepção vinda do Raspberry Pi (UART1) */
	if (huart->Instance == USART1)
	{
		uint8_t ch = uart1_rx_byte;

		if (ch == '\r' || ch == '\n')
		{
			/* Fim de comando */
			if (g_cmd_idx > 0)
			{
				/* Garante terminação em '\0' */
				if (g_cmd_idx < CMD_BUF_LEN)
				{
					g_cmd_buf[g_cmd_idx] = '\0';
				}
				else
				{
					g_cmd_buf[CMD_BUF_LEN - 1] = '\0';
				}

				/* Indica que há comando pronto para ser tratado no main() */
				g_cmd_ready = 1;
				g_cmd_idx   = 0;
			}
		}
		else
		{
			/* Acumula caracteres no buffer até o tamanho máximo */
			if (g_cmd_idx < (CMD_BUF_LEN - 1))
			{
				g_cmd_buf[g_cmd_idx++] = (char)ch;
			}
			else
			{
				/* Overflow: zera o índice (descarta comando atual) */
				g_cmd_idx = 0;
			}
		}

		/* Rearma a recepção do próximo byte em interrupção */
		HAL_UART_Receive_IT(&huart1, &uart1_rx_byte, 1);
	}
	/* Recepção vinda do terminal (UART2) */
	else if (huart->Instance == USART2)
	{
		/* Exemplo simples: ecoar o que chega de volta no terminal */
		HAL_UART_Transmit(&huart2, &uart2_rx_byte, 1, HAL_MAX_DELAY);

		/* Rearma a recepção do próximo byte em interrupção */
		HAL_UART_Receive_IT(&huart2, &uart2_rx_byte, 1);
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
