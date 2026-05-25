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
 * Wildfire Smoke Detection System
 *
 * This project uses:
 * - STM32 Nucleo F030 MCU
 * - Bosch BME688 gas/smoke sensor
 * - Reyax RYLR896 LoRa module
 *
 * The system continuously monitors gas resistance values from
 * the BME688 sensor. If smoke is detected based on a threshold,
 * an alert message is transmitted over LoRa.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bme68x.h"
#include "bme_interface.h"
#include "bme68x_defs.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
//Main BME688 device structure
struct bme68x_dev bme;
//Structure used for sensor config
struct bme68x_conf conf;
//Structure used for heater config
struct bme68x_heatr_conf heatr_conf;
//i2c address
uint8_t dev_addr;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

//Smoke Detection Threshold Settings
//Gas resistance threshold for smoke detection
#define SMOKE_THRESHOLD 50000
//Number of confirmation scans required
#define REQUIRED_HITS 3
//Total scans performed during verification
#define TOTAL_SCANS 5
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
//Flag exiting sleep mode for timer interrupt
//Volatile for flag change detection
volatile uint8_t wake_flag = 0;
//UART Reciever Buffer
uint8_t lora_rx[128];
char debug_msg[128];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//Function:LoRa_Send
//Purpose:Transmits AT commands message to receiver
//Inputs:
//msg-> Character string containing AT command
//USART1 is used for communication with the LoRa module
//HAL_UART_Transmit() is a blocking transmit function
//strlen() determines the number of bytes to send
//Must end with \"\\r\\n\" for proper AT command
void LoRa_Send(char *msg)
{

	HAL_UART_Transmit(&huart1,(uint8_t*)msg,strlen(msg),HAL_MAX_DELAY);
}

//Function: LoRa_Read
//Purpose:Reads response data coming back from the Reyax LoRa module.
//Examples of responses:+OK,+RCV=1,11,SMOKE ALERT,-45,22

void LoRa_Read(void)
{
	//Clear receive buffer before reading
	memset(lora_rx, 0, sizeof(lora_rx));

	//Receive UART data from LoRa module
	//huart1 = UART connected to Reyax module
	//1000 = timeout in milliseconds
	HAL_UART_Receive(&huart1,lora_rx,sizeof(lora_rx),1000);
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
	MX_USART1_UART_Init();
	MX_TIM6_Init();
	/* USER CODE BEGIN 2 */
	//Start the interrupt timer
	HAL_TIM_Base_Start_IT(&htim6);

	//Test communication with module
	LoRa_Send("AT\r\n");
	//Read response
	LoRa_Read();
	HAL_Delay(1000);

	//Set operating frequency to 915 MHz
	LoRa_Send("AT+BAND=915000000\r\n");
	//Read confirmation
	LoRa_Read();
	HAL_Delay(1000);

	//Set local device address to #1
	LoRa_Send("AT+ADDRESS=1\r\n");
	//Read confirmation
	LoRa_Read();
	HAL_Delay(1000);

	//Set network ID for communication (receiver and transmit must match)
	LoRa_Send("AT+NETWORKID=5\r\n");
	//Read confirmation
	LoRa_Read();
	HAL_Delay(1000);

	//Configure BME688 Sensor Interface, including I2C, read/write, and local temp
	dev_addr = BME68X_I2C_ADDR_LOW;
	bme.intf = BME68X_I2C_INTF;
	bme.intf_ptr = &dev_addr;
	bme.read = bme68x_i2c_read;
	bme.write = bme68x_i2c_write;
	bme.delay_us = bme68x_delay_us;
	bme.amb_temp = 25;

	//Initialize sensor
	bme68x_init(&bme);


	//Configure environmental sensor settings
	conf.filter = BME68X_FILTER_OFF;
	conf.odr = BME68X_ODR_NONE;
	conf.os_hum = BME68X_OS_2X;
	conf.os_pres = BME68X_OS_4X;
	conf.os_temp = BME68X_OS_8X;
	//Apply changes to sensor
	bme68x_set_conf(&conf, &bme);

	//Configure gas sensor heater
	heatr_conf.enable = BME68X_ENABLE;
	//320C heater temperature
	heatr_conf.heatr_temp = 320;
	//150ms heater duration
	heatr_conf.heatr_dur = 150;
	//Apply changes to heater hardware
	bme68x_set_heatr_conf(BME68X_FORCED_MODE,&heatr_conf,&bme);
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		//Clear Wakeup Flag
		wake_flag = 0;
		//Create Sensor Data Structures
		struct bme68x_data data;
		uint8_t n_fields;
		//Forced mode performs a single measurement.
		bme68x_set_op_mode(BME68X_FORCED_MODE,&bme);
		HAL_Delay(200);
		//Read sensor data
		bme68x_get_data(BME68X_FORCED_MODE,&data,&n_fields,&bme);

		//Requires initializing UART2, conflict with LoRa

		//Convert gas resistance reading into a text string
		//sprintf(debug_msg,"Gas Resistance: %lu\r\n",data.gas_resistance);
		//Transmit debugging information over UART
		//HAL_UART_Transmit(&huart1,(uint8_t*)debug_msg,strlen(debug_msg),HAL_MAX_DELAY)

		//Gas detection logic
		//Multiple scans are used to reduce false positives
		//Lower gas resistance values may indicate smoke or poor air quality
		uint8_t smoke_hits = 0;
		// Perform additional verification scans
		for(uint8_t i = 0; i < TOTAL_SCANS; i++)
		{
			//Start forced measurement
			bme68x_set_op_mode(BME68X_FORCED_MODE, &bme);
			//Wait for conversion
			HAL_Delay(200);
			//Read sensor data
			bme68x_get_data(BME68X_FORCED_MODE,&data,&n_fields,&bme);
			//Check smoke threshold
			if(data.gas_resistance < SMOKE_THRESHOLD)
			{
				smoke_hits++;

			}

			//Small delay between scans
			HAL_Delay(500);
		}
		//Smoke alert only transmits if enough scans exceed threshold
		if(smoke_hits >= REQUIRED_HITS)
		{
			//Reconfigure later based on desired alert
			LoRa_Send("AT+SEND=2,11,SMOKE ALERT\r\n");
		}

		//Enter Low Power Sleep Mode
		//The MCU sleeps until TIM6 generates an interrupt.
		while(wake_flag == 0)
		{
			HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON,
					PWR_SLEEPENTRY_WFI);
		}




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
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
			|RCC_CLOCKTYPE_PCLK1;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
	{
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_I2C1;
	PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
	PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */

//Function:HAL_TIM_PeriodElapsedCallback
//Purpose:Executes automatically when TIM6 overflows.
//This interrupt wakes the MCU from low power sleep mode by
//setting the wake_flag variable.
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM6)
	{
		wake_flag = 1;
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
