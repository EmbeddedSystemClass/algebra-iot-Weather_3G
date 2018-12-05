/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

/*------------------------- INCLUDED FILES ************************************/
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_tim.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_can.h"
#include "bsp.h"


/*------------------------- MACRO DEFINITIONS --------------------------------*/
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/
typedef struct 
{
  GPIO_TypeDef* port;
  uint16_t pin;
  GPIO_PinState active_state;
} bsp_led_t;

typedef struct
{
  uint32_t mode;
  uint32_t press_level;
  bsp_button_callback_t callback;
  GPIO_TypeDef* port;
  uint16_t pin;
} bsp_button_t;

/*------------------------- PUBLIC VARIABLES ---------------------------------*/
TIM_HandleTypeDef htim1; 
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

SPI_HandleTypeDef bsp_sd_spi_handle;

volatile unsigned long BspHighFrequencyTimerTicks = 0;

/*------------------------- PRIVATE VARIABLES --------------------------------*/

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/
static void BSP_Clock_Configure(void);
static void BSP_Uart_Configure(void);
static void BSP_Can_Configure(void);
static void BSP_WiFi_Pins_Configure(void);
static void BSP_SD_Pins_Configure(void);
static void BSP_SD_SPI_Configure(void);
static void BSP_WD_Configure(void);
static void BSP_WIFI_IRQ_Configure(void);
static void BSP_GPIO_Configure(void);;

static void _Error_Handler(char * file, int line);
/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
void BSP_init(void)
{
  /* Reset of all peripherals */
  HAL_Init();
  
  /* Configure subsystems */
  BSP_Clock_Configure();
  BSP_GPIO_Configure();
  BSP_Uart_Configure();
  BSP_WiFi_Pins_Configure();
  BSP_WIFI_IRQ_Configure();
  BSP_WD_Configure();
  BSP_Can_Configure();
  BSP_SD_Pins_Configure();
  BSP_SD_SPI_Configure();
}


void BSP_WD_Feed(uint32_t instance)
{
  if(instance == BSP_WD_EXTERNAL)
  {
#if 0
    HAL_GPIO_WritePin(BSP_WD_GPIO_PORT, BSP_WD_PIN, GPIO_PIN_RESET);
    BSP_Delay(100);
    HAL_GPIO_WritePin(BSP_WD_GPIO_PORT, BSP_WD_PIN, GPIO_PIN_SET);
#else
    HAL_GPIO_TogglePin(BSP_WD_GPIO_PORT, BSP_WD_PIN);
#endif
    
  }
}

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

/** System Clock Configuration
*/
static void BSP_Clock_Configure(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

    /**Configure the main internal regulator output voltage 
    */
  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
  
    /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  
  /* UART clock enable */
#if (BSP_UART1_ENABLED == BSP_ON)
  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_USART1_FORCE_RESET();
  __HAL_RCC_USART1_RELEASE_RESET();
#endif
  
#if (BSP_UART2_ENABLED == BSP_ON)
  __HAL_RCC_USART2_CLK_ENABLE();
  __HAL_RCC_USART2_FORCE_RESET();
  __HAL_RCC_USART2_RELEASE_RESET();
#endif
  
#if (BSP_UART3_ENABLED == BSP_ON)
  __HAL_RCC_USART3_CLK_ENABLE();
  __HAL_RCC_USART3_FORCE_RESET();
  __HAL_RCC_USART3_RELEASE_RESET();
#endif

#if (BSP_UART5_ENABLED == BSP_ON)
  __HAL_RCC_UART5_CLK_ENABLE();
  __HAL_RCC_UART5_FORCE_RESET();
  __HAL_RCC_UART5_RELEASE_RESET();
#endif


#if (BSP_USART3_ENABLED == BSP_ON)
  __HAL_RCC_USART3_CLK_ENABLE();
  __HAL_RCC_USART3_FORCE_RESET();
  __HAL_RCC_USART3_RELEASE_RESET();
#endif

#if (BSP_USART6_ENABLED == BSP_ON)
  __HAL_RCC_USART6_CLK_ENABLE();
  __HAL_RCC_USART6_FORCE_RESET();
  __HAL_RCC_USART6_RELEASE_RESET();
#endif
  
#if (BSP_UART7_ENABLED == BSP_ON)
  __HAL_RCC_UART7_CLK_ENABLE();
  __HAL_RCC_UART7_FORCE_RESET();
  __HAL_RCC_UART7_RELEASE_RESET();
#endif
  
  /* TIMER3 clock enable */
  __HAL_RCC_TIM2_CLK_ENABLE();
  __HAL_RCC_TIM3_CLK_ENABLE();

  /* SPI1 clock enable */
  __HAL_RCC_SPI1_CLK_ENABLE();
  
  /* SPI1 clock enable */
  __HAL_RCC_SPI2_CLK_ENABLE();
  
    /* CAN clock enable */
  __HAL_RCC_CAN1_CLK_ENABLE();
}

static void BSP_GPIO_Configure(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GNSS_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|SD_CS_Pin 
                          |GNSS_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, MDM_PWR_ON_Pin|MDM_RESET_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  //HAL_GPIO_WritePin(GPIOD, MDM_DTR_Pin|WIFI_SPI_CS_Pin|RS485_DE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  //HAL_GPIO_WritePin(GPIOB, WIFI_HIB_Pin|WIFI_RESET_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : CAN_ADR_0_Pin CAN_ADR_1_Pin CAN_ADR_2_Pin */
  GPIO_InitStruct.Pin = CAN_ADR_0_Pin|CAN_ADR_1_Pin|CAN_ADR_2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PE11 PE12 PE13 SD_CS_Pin  */
  GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|SD_CS_Pin 
                          |GNSS_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : GNSS_EN_Pin */
  GPIO_InitStruct.Pin = GNSS_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : SD_CD_Pin GNSS_DRDY_Pin */
  GPIO_InitStruct.Pin = SD_CD_Pin|GNSS_DRDY_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : SD_SCLK_Pin SD_DO_Pin SD_DI_Pin */
  GPIO_InitStruct.Pin = SD_SCLK_Pin|SD_DO_Pin|SD_DI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : MDM_PWR_ON_Pin MDM_RESET_Pin */
  HAL_GPIO_WritePin(GPIOD, MDM_PWR_ON_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOD, MDM_RESET_Pin, GPIO_PIN_SET);
  GPIO_InitStruct.Pin = MDM_PWR_ON_Pin|MDM_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : MDM_RI_Pin WIFI_INT_Pin */
  GPIO_InitStruct.Pin = MDM_RI_Pin|WIFI_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : WIFI_HIB_Pin */
  GPIO_InitStruct.Pin = WIFI_HIB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : WIFI_RESET_Pin */
  GPIO_InitStruct.Pin = WIFI_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  HAL_GPIO_TogglePin(WIFI_RESET_GPIO_Port, WIFI_RESET_Pin);

  /*Configure GPIO pins : I2C1_SCL_Pin I2C1_SDA_Pin */
  GPIO_InitStruct.Pin = I2C1_SCL_Pin|I2C1_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}


static void BSP_WD_Configure(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  
  /*Configure GPIO pin : WD_PING_Pin */
  HAL_GPIO_WritePin(BSP_WD_GPIO_PORT, BSP_WD_PIN, GPIO_PIN_SET);
  GPIO_InitStruct.Pin = BSP_WD_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BSP_WD_GPIO_PORT, &GPIO_InitStruct);
}

static void BSP_Uart_Configure(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  
#if (BSP_USART1_ENABLED == BSP_ON)
  {
    /* Peripheral clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();
  
    /**USART3 GPIO Configuration    
    PA9     ------> USART3_TX
    PA10     ------> USART3_RX
    */
    GPIO_InitStruct.Pin = BSP_USART1_TX_PIN|BSP_USART1_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(BSP_USART1_TX_PORT, &GPIO_InitStruct);

    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    HAL_NVIC_ClearPendingIRQ(USART1_IRQn);
  }
#endif
  
#if (BSP_USART3_ENABLED == BSP_ON)
  {
    /* Peripheral clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();
  
    /**USART3 GPIO Configuration    
    PD8     ------> USART3_TX
    PD9     ------> USART3_RX
    PD11     ------> USART3_CTS
    PD12     ------> USART3_RTS 
    */
    GPIO_InitStruct.Pin = BSP_USART3_TX_PIN|BSP_USART3_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(BSP_USART3_TX_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = BSP_USART3_CTS_PIN|BSP_USART3_RTS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(BSP_USART3_CTS_PORT, &GPIO_InitStruct);
    
      /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(USART3_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
    HAL_NVIC_ClearPendingIRQ(USART3_IRQn);
  }
#endif
  
#if (BSP_USART6_ENABLED == BSP_ON)  
  {
    /* Peripheral clock enable */
    __HAL_RCC_USART6_CLK_ENABLE();
  
    /**USART6 GPIO Configuration    
    PC6     ------> USART6_TX
    PC7     ------> USART6_RX 
    */
    GPIO_InitStruct.Pin = BSP_USART6_TX_PIN|BSP_USART6_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
    HAL_GPIO_Init(BSP_USART6_TX_PORT, &GPIO_InitStruct);
    
    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(USART6_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USART6_IRQn);
    HAL_NVIC_ClearPendingIRQ(USART6_IRQn);
  }
#endif

#if (BSP_UART7_ENABLED == BSP_ON)  
  {
    /* Peripheral clock enable */
    __HAL_RCC_UART7_CLK_ENABLE();
  
    /**USART6 GPIO Configuration    
    PE8     ------> USART7_TX
    PE7     ------> USART7_RX 
    */
    GPIO_InitStruct.Pin = BSP_UART7_TX_PIN|BSP_UART7_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART7;
    HAL_GPIO_Init(BSP_UART7_TX_PORT, &GPIO_InitStruct);
    
    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(UART7_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(UART7_IRQn);
    HAL_NVIC_ClearPendingIRQ(UART7_IRQn);
  }
#endif
}

void BSP_WiFi_Pins_Configure(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  GPIO_InitStruct.Pin = WIFI_SCLK_Pin|WIFI_SPI_DOUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = WIFI_SPI_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = WIFI_SPI_DIN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);  
}


void BSP_SD_Pins_Configure(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  /**SPI2 GPIO Configuration    
  PB10     ------> SPI2_SCK
  PB14     ------> SPI2_MISO
  PB15     ------> SPI2_MOSI 
  */
  GPIO_InitStruct.Pin = SD_SCLK_Pin|SD_DO_Pin|SD_DI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = SD_SCLK_Pin|SD_DO_Pin|SD_DI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  /**Configure GPIO pins : 
    PE14     ------> SD_CS_Pin 
  */
  GPIO_InitStruct.Pin = SD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
  
  /**Configure GPIO pins : 
    PE15     ------> SD_CD_Pin 
  */
  GPIO_InitStruct.Pin = SD_CD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}

void BSP_SD_SPI_Configure(void)
{
  /* SPI2 parameter configuration*/
  bsp_sd_spi_handle.Instance = SPI2;
  bsp_sd_spi_handle.Init.Mode = SPI_MODE_MASTER;
  bsp_sd_spi_handle.Init.Direction = SPI_DIRECTION_2LINES;
  bsp_sd_spi_handle.Init.DataSize = SPI_DATASIZE_8BIT;
  bsp_sd_spi_handle.Init.CLKPolarity = SPI_POLARITY_LOW;
  bsp_sd_spi_handle.Init.CLKPhase = SPI_PHASE_1EDGE;
  bsp_sd_spi_handle.Init.NSS = SPI_NSS_SOFT;
  bsp_sd_spi_handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  bsp_sd_spi_handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
  bsp_sd_spi_handle.Init.TIMode = SPI_TIMODE_DISABLE;
  bsp_sd_spi_handle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  bsp_sd_spi_handle.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&bsp_sd_spi_handle) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }
}

void BSP_Can_Configure(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  /**CAN1 GPIO Configuration    
  PD0     ------> CAN1_RX
  PD1     ------> CAN1_TX 
  */
 
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* Peripheral interrupt init */
  HAL_NVIC_SetPriority(CAN1_TX_IRQn, 11, 0);
  HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
  HAL_NVIC_ClearPendingIRQ(CAN1_TX_IRQn);
  
  HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 12, 0);
  HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
  HAL_NVIC_ClearPendingIRQ(CAN1_RX0_IRQn);
 
  HAL_NVIC_SetPriority(CAN1_RX1_IRQn, 13, 0);
  HAL_NVIC_EnableIRQ(CAN1_RX1_IRQn);
  HAL_NVIC_ClearPendingIRQ(CAN1_RX1_IRQn);
  
  HAL_NVIC_SetPriority(CAN1_SCE_IRQn, 14, 0);
  HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn); 
  HAL_NVIC_ClearPendingIRQ(CAN1_SCE_IRQn);
}

static void BSP_WIFI_IRQ_Configure(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;
    
  /* Set pin as input */
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pin = WIFI_INT_Pin;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    
  HAL_NVIC_SetPriority(EXTI2_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);
}


/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1) 
  {
    HAL_IncTick();
  }
  else if(htim->Instance == TIM2)
  {
    BspHighFrequencyTimerTicks++;
  }
}


/**
* @brief This function handles TIM1 update and TIM16 interrupts.
*/
void TIM1_UP_TIM16_IRQHandler(void)
{
  /* USER CODE BEGIN TIM1_UP_TIM16_IRQn 0 */

  /* USER CODE END TIM1_UP_TIM16_IRQn 0 */
  HAL_TIM_IRQHandler(&htim1);
  /* USER CODE BEGIN TIM1_UP_TIM16_IRQn 1 */

  /* USER CODE END TIM1_UP_TIM16_IRQn 1 */
}


#if defined(RTOS_TICKS_HAL_USE)
/**
  * @brief  This function configures the TIM1 as a time base source. 
  *         The time source is configured  to have 1ms time base with a dedicated 
  *         Tick interrupt priority. 
  * @note   This function is called  automatically at the beginning of program after
  *         reset by HAL_Init() or at any time when clock is configured, by HAL_RCC_ClockConfig(). 
  * @param  TickPriority: Tick interrupt priorty.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
  RCC_ClkInitTypeDef    clkconfig;
  uint32_t              uwTimclock = 0;
  uint32_t              uwPrescalerValue = 0;
  uint32_t              pFLatency;
  
  /*Configure the TIM1 IRQ priority */
  HAL_NVIC_SetPriority(TIM1_UP_TIM16_IRQn, TickPriority ,0); 
  
  /* Enable the TIM1 global Interrupt */
  HAL_NVIC_EnableIRQ(TIM1_UP_TIM16_IRQn); 
  
  /* Enable TIM1 clock */
  __HAL_RCC_TIM1_CLK_ENABLE();
  
  /* Get clock configuration */
  HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);
  
  /* Compute TIM1 clock */
  uwTimclock = HAL_RCC_GetPCLK2Freq();
   
  /* Compute the prescaler value to have TIM1 counter clock equal to 1MHz */
  uwPrescalerValue = (uint32_t) ((uwTimclock / 1000000) - 1);
  
  /* Initialize TIM1 */
  htim1.Instance = TIM1;
  
  /* Initialize TIMx peripheral as follow:
  + Period = [(TIM1CLK/1000) - 1]. to have a (1/1000) s time base.
  + Prescaler = (uwTimclock/1000000 - 1) to have a 1MHz counter clock.
  + ClockDivision = 0
  + Counter direction = Up
  */
  htim1.Init.Period = (1000000 / 1000) - 1;
  htim1.Init.Prescaler = uwPrescalerValue;
  htim1.Init.ClockDivision = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  if(HAL_TIM_Base_Init(&htim1) == HAL_OK)
  {
    /* Start the TIM time Base generation in interrupt mode */
    return HAL_TIM_Base_Start_IT(&htim1);
  }
  
  /* Return function status */
  return HAL_ERROR;
}
#endif // #if defined(RTOS_TICKS_HAL_USE)

/**
  * @brief  Suspend Tick increment.
  * @note   Disable the tick increment by disabling TIM1 update interrupt.
  * @param  None
  * @retval None
  */
void HAL_SuspendTick(void)
{
  /* Disable TIM1 update Interrupt */
  __HAL_TIM_DISABLE_IT(&htim1, TIM_IT_UPDATE);                                                  
}

/**
  * @brief  Resume Tick increment.
  * @note   Enable the tick increment by Enabling TIM1 update interrupt.
  * @param  None
  * @retval None
  */
void HAL_ResumeTick(void)
{
  /* Enable TIM1 Update interrupt */
  __HAL_TIM_ENABLE_IT(&htim1, TIM_IT_UPDATE);
}



/* delay in microseconds (resolution 100us) */
void BSP_Delay(uint32_t microseconds)
{
  uint32_t tickstart = BspHighFrequencyTimerTicks;
  uint32_t tickdelay = microseconds/100;
  if(tickdelay > 0)
  {
    while((BspHighFrequencyTimerTicks - tickstart) < tickdelay)
    {
    }
  }
}

void _Error_Handler(char * file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler_Debug */ 
}



