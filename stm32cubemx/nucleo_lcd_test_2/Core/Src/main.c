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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEMO_WAKE_SECONDS 2U
#define SHT40_I2C_ADDR         (0x44U << 1)
#define SHT40_CMD_MEASURE_HIGH 0xFDU

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c2;

LCD_HandleTypeDef hlcd;

RTC_HandleTypeDef hrtc;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
volatile uint8_t g_rtc_wakeup = 0U;
volatile uint8_t g_use_lse = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_LCD_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_RTC_Init(void);
static void MX_I2C2_Init(void);
/* USER CODE BEGIN PFP */
static void LCD_DisplayNumber3(uint16_t value);
static uint16_t MockHumidityX10(void);
static void DebugPrint(const char *text);
static void DebugPrintf(const char *fmt, ...);
static void I2C2_ScanBus(void);
static uint8_t SHT40_Crc8(const uint8_t *data, uint8_t len);
static HAL_StatusTypeDef SHT40_ReadRaw(uint16_t *temperature_ticks, uint16_t *humidity_ticks);
static void SHT40_TestLoop(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void DebugPrint(const char *text)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)text, (uint16_t)strlen(text), HAL_MAX_DELAY);
}

static void DebugPrintf(const char *fmt, ...)
{
  char buffer[128];
  int len;
  va_list args;

  va_start(args, fmt);
  len = vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  if (len <= 0)
  {
    return;
  }

  if ((size_t)len > sizeof(buffer))
  {
    len = (int)sizeof(buffer);
  }

  HAL_UART_Transmit(&huart2, (uint8_t *)buffer, (uint16_t)len, HAL_MAX_DELAY);
}

static void I2C2_ScanBus(void)
{
  uint8_t addr;
  uint8_t found = 0U;

  DebugPrint("Scanning I2C2 addresses 0x08..0x77\r\n");

  for (addr = 0x08U; addr <= 0x77U; addr++)
  {
    if (HAL_I2C_IsDeviceReady(&hi2c2, (uint16_t)(addr << 1), 2U, 20U) == HAL_OK)
    {
      DebugPrintf("I2C device found at 0x%02X\r\n", addr);
      found = 1U;
    }
  }

  if (found == 0U)
  {
    DebugPrint("No I2C devices found\r\n");
  }
}

static uint8_t SHT40_Crc8(const uint8_t *data, uint8_t len)
{
  uint8_t crc = 0xFFU;
  uint8_t i;
  uint8_t bit;

  for (i = 0U; i < len; i++)
  {
    crc ^= data[i];
    for (bit = 0U; bit < 8U; bit++)
    {
      if ((crc & 0x80U) != 0U)
      {
        crc = (uint8_t)((crc << 1) ^ 0x31U);
      }
      else
      {
        crc <<= 1;
      }
    }
  }

  return crc;
}

static HAL_StatusTypeDef SHT40_ReadRaw(uint16_t *temperature_ticks, uint16_t *humidity_ticks)
{
  uint8_t cmd = SHT40_CMD_MEASURE_HIGH;
  uint8_t rx[6];

  if (HAL_I2C_Master_Transmit(&hi2c2, SHT40_I2C_ADDR, &cmd, 1U, 100U) != HAL_OK)
  {
    return HAL_ERROR;
  }

  HAL_Delay(10U);

  if (HAL_I2C_Master_Receive(&hi2c2, SHT40_I2C_ADDR, rx, sizeof(rx), 100U) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (SHT40_Crc8(&rx[0], 2U) != rx[2] || SHT40_Crc8(&rx[3], 2U) != rx[5])
  {
    return HAL_ERROR;
  }

  *temperature_ticks = (uint16_t)(((uint16_t)rx[0] << 8) | rx[1]);
  *humidity_ticks = (uint16_t)(((uint16_t)rx[3] << 8) | rx[4]);

  return HAL_OK;
}

static void SHT40_TestLoop(void)
{
  uint16_t temperature_ticks;
  uint16_t humidity_ticks;
  float temperature_c;
  float humidity_rh;

  if (HAL_I2C_IsDeviceReady(&hi2c2, SHT40_I2C_ADDR, 3U, 100U) != HAL_OK)
  {
    DebugPrint("SHT40 not responding at 0x44\r\n");
    HAL_Delay(1000U);
    return;
  }

  if (SHT40_ReadRaw(&temperature_ticks, &humidity_ticks) != HAL_OK)
  {
    DebugPrint("SHT40 read failed\r\n");
    HAL_Delay(1000U);
    return;
  }

  temperature_c = -45.0f + (175.0f * (float)temperature_ticks / 65535.0f);
  humidity_rh = -6.0f + (125.0f * (float)humidity_ticks / 65535.0f);

  if (humidity_rh < 0.0f)
  {
    humidity_rh = 0.0f;
  }
  else if (humidity_rh > 100.0f)
  {
    humidity_rh = 100.0f;
  }

  DebugPrintf("SHT40 OK  T=%.2f C  RH=%.2f %%\r\n", temperature_c, humidity_rh);
  HAL_Delay(1000U);
}

static uint32_t LCD_ComposeSegBit(uint8_t seg_index)
{
  /* These are the 6 SEG lines currently wired to the glass. */
  static const uint8_t seg_bits[6] = {0U, 5U, 7U, 8U, 9U, 10U};
  return (1UL << seg_bits[seg_index]);
}

static uint16_t MockHumidityX10(void)
{
  static uint32_t lcg = 0x1A2B3C4DU;
  lcg = (lcg * 1664525UL) + 1013904223UL;
  return (uint16_t)(lcg % 1000UL); /* 0.0 .. 99.9 %RH */
}

static void LCD_DisplayNumber3(uint16_t value)
{
  /* 7-segment font: bits = a b c d e f g in bit positions 0..6. */
  static const uint8_t font7[10] =
  {
    0x3FU, /* 0 */
    0x06U, /* 1 */
    0x5BU, /* 2 */
    0x4FU, /* 3 */
    0x66U, /* 4 */
    0x6DU, /* 5 */
    0x7DU, /* 6 */
    0x07U, /* 7 */
    0x7FU, /* 8 */
    0x6FU  /* 9 */
  };
  uint8_t d_right = (uint8_t)(value % 10U);
  uint8_t d_mid = (uint8_t)((value / 10U) % 10U);
  uint8_t d_left = (uint8_t)((value / 100U) % 10U);
  /* Physical digit order on glass is mirrored versus SEG pair order. */
  uint8_t digits[3] = {d_left, d_mid, d_right};
  uint8_t pos;
  uint32_t com_data[4] = {0U, 0U, 0U, 0U};
  uint32_t seg_mask = LCD_ComposeSegBit(0) | LCD_ComposeSegBit(1) |
                      LCD_ComposeSegBit(2) | LCD_ComposeSegBit(3) |
                      LCD_ComposeSegBit(4) | LCD_ComposeSegBit(5);

  /* Per your measured wiring, each digit has 2 SEG lines:
   * line_abc drives {a,b,c} = {top, top-right, bottom-right}
   * line_defg drives {d,e,f,g} = {bottom, bottom-left, top-left, middle}
   *
   * Exact pin map provided:
   * LCD pin1 -> A1  -> SEG0  -> digit3 abc (left)
   * LCD pin2 -> A3  -> SEG5  -> digit3 defg (left)
   * LCD pin3 -> D3  -> SEG7  -> digit2 abc (middle)
   * LCD pin4 -> D5  -> SEG8  -> digit2 defg (middle)
   * LCD pin5 -> D4  -> SEG9  -> digit1 abc (right)
   * LCD pin6 -> D6  -> SEG10 -> digit1 defg (right)
   *
   * Therefore (right,middle,left):
   * abc lines  = SEG9, SEG7, SEG0
   * defg lines = SEG10, SEG8, SEG5
   *
   * COM mapping from your notes:
   * COM0=d
   * COM1=c/e
   * COM2=b/g
   * COM3=a/f
   */
  {
    uint8_t abc_seg_idx[3] = {4U, 2U, 0U};
    uint8_t defg_seg_idx[3] = {5U, 3U, 1U};

  for (pos = 0U; pos < 3U; pos++)
  {
    uint8_t p = font7[digits[pos]];
    uint32_t line_abc = LCD_ComposeSegBit(abc_seg_idx[pos]);
    uint32_t line_defg = LCD_ComposeSegBit(defg_seg_idx[pos]);

    if ((p & (1U << 3)) != 0U) com_data[0] |= line_defg; /* d -> COM0 via defg line */
    if ((p & (1U << 2)) != 0U) com_data[1] |= line_abc;  /* c -> COM1 via abc line */
    if ((p & (1U << 4)) != 0U) com_data[1] |= line_defg; /* e -> COM1 via defg line */
    if ((p & (1U << 1)) != 0U) com_data[2] |= line_abc;  /* b -> COM2 via abc line */
    if ((p & (1U << 6)) != 0U) com_data[2] |= line_defg; /* g -> COM2 via defg line */
    if ((p & (1U << 0)) != 0U) com_data[3] |= line_abc;  /* a -> COM3 via abc line */
    if ((p & (1U << 5)) != 0U) com_data[3] |= line_defg; /* f -> COM3 via defg line */
  }
  }

  if (HAL_LCD_Clear(&hlcd) != HAL_OK) Error_Handler();
  if (HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER0, seg_mask, com_data[0]) != HAL_OK) Error_Handler();
  if (HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER2, seg_mask, com_data[1]) != HAL_OK) Error_Handler();
  if (HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER4, seg_mask, com_data[2]) != HAL_OK) Error_Handler();
  if (HAL_LCD_Write(&hlcd, LCD_RAM_REGISTER6, seg_mask, com_data[3]) != HAL_OK) Error_Handler();
  if (HAL_LCD_UpdateDisplayRequest(&hlcd) != HAL_OK) Error_Handler();
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc_cb)
{
  (void)hrtc_cb;
  g_rtc_wakeup = 1U;
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
  MX_RTC_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */
  DebugPrint("\r\nBooting SHT40 test\r\n");
  DebugPrint("LCD init disabled for this test build\r\n");
  I2C2_ScanBus();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    SHT40_TestLoop();
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_5;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_RTC;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x00000608;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief LCD Initialization Function
  * @param None
  * @retval None
  */
static void MX_LCD_Init(void)
{

  /* USER CODE BEGIN LCD_Init 0 */

  /* USER CODE END LCD_Init 0 */

  /* USER CODE BEGIN LCD_Init 1 */
  /* For NUCLEO default SB45(VBAT/VLCD)=ON path, use external VLCD source. */
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_SYSCFG_VLCD_CAPA_CONFIG(SYSCFG_VLCD_PB2_EXT_CAPA_ON);

  /* USER CODE END LCD_Init 1 */
  hlcd.Instance = LCD;
  hlcd.Init.Prescaler = LCD_PRESCALER_1;
  hlcd.Init.Divider = LCD_DIVIDER_16;
  hlcd.Init.Duty = LCD_DUTY_1_4;
  hlcd.Init.Bias = LCD_BIAS_1_2;
  hlcd.Init.VoltageSource = LCD_VOLTAGESOURCE_INTERNAL;
  hlcd.Init.Contrast = LCD_CONTRASTLEVEL_0;
  hlcd.Init.DeadTime = LCD_DEADTIME_0;
  hlcd.Init.PulseOnDuration = LCD_PULSEONDURATION_0;
  hlcd.Init.HighDrive = LCD_HIGHDRIVE_0;
  hlcd.Init.BlinkMode = LCD_BLINKMODE_OFF;
  hlcd.Init.BlinkFrequency = LCD_BLINKFREQUENCY_DIV8;
  hlcd.Init.MuxSegment = LCD_MUXSEGMENT_DISABLE;
  if (HAL_LCD_Init(&hlcd) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LCD_Init 2 */

  /* USER CODE END LCD_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the WakeUp
  */
  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

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
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
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
