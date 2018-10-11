/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _BSP_H
#define _BSP_H

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/
 
#define BSP_OFF 0
#define BSP_ON  1
  
#define BSP_PIN_HIGH    GPIO_PIN_SET
#define BSP_PIN_LOW     GPIO_PIN_RESET
  
/* reset reasons */
#define BSP_RESET_UNKNOWN   0              /* ???? wtf reset */
#define BSP_RESET_PINRST    1              /* PIN reset flag (external watchdog or reset button) */
#define BSP_RESET_PORRST    2              /* POR/PDR (power on) reset flag */
#define BSP_RESET_SFTRST    3              /* Software Reset flag */
#define BSP_RESET_IWDGRST   4              /* Independent Watchdog reset flag */
#define BSP_RESET_WWDGRST   5              /* Window watchdog reset flag */
#define BSP_RESET_LPWRRST   6              /* Low-Power reset flag */
  
/* watchdog */
#define BSP_WD_INTERNAL 0
#define BSP_WD_EXTERNAL 1
#define BSP_WD_PIN       GPIO_PIN_0
#define BSP_WD_GPIO_PORT GPIOC
  


  
/* UART definitions */
#define BSP_UART_COUNT  4
  
#if !defined(BSP_USART1_ENABLED)  
#define BSP_USART1_ENABLED       BSP_ON
#endif
#if !defined(BSP_USART3_ENABLED)
#define BSP_USART3_ENABLED       BSP_ON
#endif
#if !defined(BSP_USART6_ENABLED)
#define BSP_USART6_ENABLED       BSP_ON
#endif
#if !defined(BSP_UART7_ENABLED)
#define BSP_UART7_ENABLED        BSP_ON
#endif
  
#if !defined(BSP_USART3_HW_CTRL)
/* enable it by default - modem should be on USART3 */
#define BSP_USART3_HW_CTRL BSP_ON
#endif
  
/* USART1 pins */  
#if (BSP_USART1_ENABLED == BSP_ON) 
#define BSP_USART1_TX_PORT      GPIOA
#define BSP_USART1_TX_PIN       GPIO_PIN_9
#define BSP_USART1_RX_PORT      GPIOA
#define BSP_USART1_RX_PIN       GPIO_PIN_10
#if (BSP_USART1_HW_CTRL == BSP_ON)
#define BSP_USART1_RTS_PORT     not_defined
#define BSP_USART1_RTS_PIN      not_defined  
#define BSP_USART1_CTS_PORT     not_defined
#define BSP_USART1_CTS_PIN      not_defined
#endif 
#endif
  
/* USART3 pins */  
#if (BSP_USART3_ENABLED == BSP_ON)
#define BSP_USART3_TX_PORT      GPIOD
#define BSP_USART3_TX_PIN       GPIO_PIN_8
#define BSP_USART3_RX_PORT      GPIOD
#define BSP_USART3_RX_PIN       GPIO_PIN_9
#if (BSP_USART3_HW_CTRL == BSP_ON)
#define BSP_USART3_RTS_PORT     GPIOD
#define BSP_USART3_RTS_PIN      GPIO_PIN_12  
#define BSP_USART3_CTS_PORT     GPIOD
#define BSP_USART3_CTS_PIN      GPIO_PIN_11
#endif 
#endif
  

/* USART6 pins */  
#if (BSP_USART6_ENABLED == BSP_ON)
#define BSP_USART6_TX_PORT      GPIOC
#define BSP_USART6_TX_PIN       GPIO_PIN_6
#define BSP_USART6_RX_PORT      GPIOC
#define BSP_USART6_RX_PIN       GPIO_PIN_7
#if (BSP_USART6_HW_CTRL == BSP_ON)
#define BSP_USART6_RTS_PORT     not_defined
#define BSP_USART6_RTS_PIN      not_defined  
#define BSP_USART6_CTS_PORT     not_defined
#define BSP_USART6_CTS_PIN      not_defined
#endif 
#endif
  
  
/* UART7 pins */  
#if (BSP_UART7_ENABLED == BSP_ON)
#define BSP_UART7_TX_PORT      GPIOE
#define BSP_UART7_TX_PIN       GPIO_PIN_8
#define BSP_UART7_RX_PORT      GPIOE
#define BSP_UART7_RX_PIN       GPIO_PIN_7
#if (BSP_UART7_HW_CTRL == BSP_ON)
#define BSP_UART7_RTS_PORT     not_defined
#define BSP_UART7_RTS_PIN      not_defined  
#define BSP_UART7_CTS_PORT     not_defined
#define BSP_UART7_CTS_PIN      not_defined
#endif   
#endif
  
  
/* UART register rw macros */
  /* @TODO: CPU dependent, not board; probably should go to MSP */
#define BSP_USART_READ_DATA(USARTx)             ((USARTx)->DR)
#define BSP_USART_WRITE_DATA(USARTx, data)      ((USARTx)->DR = (data))
#define BSP_USART_TXEMPTY(USARTx)               ((USARTx)->SR & USART_SR_TXE)
#define BSP_USART_WAIT(USARTx)                  while (!BSP_USART_TXEMPTY(USARTx))
  
#define BSP_UART_START_TX(USARTx)               SET_BIT((USARTx)->CR1, USART_CR1_TXEIE)
#define BSP_UART_STOP_TX(USARTx)                CLEAR_BIT((USARTx)->CR1, USART_CR1_TXEIE)
  
#define BSP_USART_RX_INT(USARTx)                ((USARTx)->SR & USART_SR_RXNE)
#define BSP_USART_TX_INT(USARTx)                ((USARTx)->SR & USART_SR_TXE)

/** @brief  Enable the specified Usart interrupts.
  * @param  __HANDLE__: specifies the USART Handle.
  *         USART Handle selects the USARTx peripheral (USART availability and x value depending on device).
  * @param  __INTERRUPT__: specifies the USART interrupt source to enable.
  *          This parameter can be one of the following values:
  *            @arg USART_IT_TXE:  Transmit Data Register empty interrupt
  *            @arg USART_IT_TC:   Transmission complete interrupt
  *            @arg USART_IT_RXNE: Receive Data register not empty interrupt
  *            @arg USART_IT_IDLE: Idle line detection interrupt
  *            @arg USART_IT_PE:   Parity Error interrupt
  *            @arg USART_IT_ERR:  Error interrupt(Frame error, noise error, overrun error)
  * @retval None
  */
#if 0
#define BSP_UART_ENABLE_IT(__UART__, __INTERRUPT__)   ((((__INTERRUPT__) >> 28) == USART_CR1_REG_INDEX)? ((__UART__)->CR1 |= ((__INTERRUPT__) & USART_IT_MASK)): \
                                                       (((__INTERRUPT__) >> 28) == USART_CR2_REG_INDEX)? ((__UART__)->CR2 |=  ((__INTERRUPT__) & USART_IT_MASK)): \
                                                       ((__UART__)->CR3 |= ((__INTERRUPT__) & USART_IT_MASK)))
#endif

/** @brief  Disable the specified Usart interrupts.
  * @param  __HANDLE__: specifies the USART Handle.
  *         USART Handle selects the USARTx peripheral (USART availability and x value depending on device).
  * @param  __INTERRUPT__: specifies the USART interrupt source to disable.
  *          This parameter can be one of the following values:
  *            @arg USART_IT_TXE:  Transmit Data Register empty interrupt
  *            @arg USART_IT_TC:   Transmission complete interrupt
  *            @arg USART_IT_RXNE: Receive Data register not empty interrupt
  *            @arg USART_IT_IDLE: Idle line detection interrupt
  *            @arg USART_IT_PE:   Parity Error interrupt
  *            @arg USART_IT_ERR:  Error interrupt(Frame error, noise error, overrun error)
  * @retval None
  */
#if 0
#define BSP_UART_DISABLE_IT(__UART__, __INTERRUPT__)  ((((__INTERRUPT__) >> 28) == USART_CR1_REG_INDEX)? ((__UART__)->CR1 &= ~((__INTERRUPT__) & USART_IT_MASK)): \
                                                       (((__INTERRUPT__) >> 28) == USART_CR2_REG_INDEX)? ((__UART__)->CR2 &= ~((__INTERRUPT__) & USART_IT_MASK)): \
                                                       ((__UART__)->CR3 &= ~ ((__INTERRUPT__) & USART_IT_MASK)))
#endif

  
/* Si70xx temperature/humidity sensors */
#define BSP_SI70XX_SENS_CNT   1  
  
#define BSP_SI70XX_SENS_0     0
#define BSP_SI70XX_SENS_0_I2C I2C2
    
/* CAN definitions */
#define BSP_CAN_COUNT  1
  
#define CAN_ADR_0_Pin GPIO_PIN_0
#define CAN_ADR_0_GPIO_Port GPIOB
#define CAN_ADR_1_Pin GPIO_PIN_1
#define CAN_ADR_1_GPIO_Port GPIOB
#define CAN_ADR_2_Pin GPIO_PIN_2
#define CAN_ADR_2_GPIO_Port GPIOB
#define CAN_RX_Pin GPIO_PIN_0
#define CAN_RX_GPIO_Port GPIOD
#define CAN_TX_Pin GPIO_PIN_1
#define CAN_TX_GPIO_Port GPIOD
  
  
/* SD Card pins */
#define SD_CS_Pin         GPIO_PIN_14
#define SD_CS_GPIO_Port   GPIOE
#define SD_CD_Pin         GPIO_PIN_15
#define SD_CD_GPIO_Port   GPIOE
#define SD_SCLK_Pin       GPIO_PIN_10
#define SD_SCLK_GPIO_Port GPIOB
#define SD_DO_Pin         GPIO_PIN_14
#define SD_DO_GPIO_Port   GPIOB
#define SD_DI_Pin         GPIO_PIN_15
#define SD_DI_GPIO_Port   GPIOB  
  
  
/* Modem pins */
#define MDM_TXD_Pin           GPIO_PIN_8
#define MDM_TXD_GPIO_Port     GPIOD
#define MDM_RXD_Pin           GPIO_PIN_9
#define MDM_RXD_GPIO_Port     GPIOD
#define MDM_PWR_ON_Pin        GPIO_PIN_13
#define MDM_PWR_ON_GPIO_Port  GPIOD
#define MDM_CTS_Pin           GPIO_PIN_11
#define MDM_CTS_GPIO_Port     GPIOD
#define MDM_RTS_Pin           GPIO_PIN_12
#define MDM_RTS_GPIO_Port     GPIOD
#define MDM_RESET_Pin         GPIO_PIN_10
#define MDM_RESET_GPIO_Port   GPIOD
#define MDM_DTR_Pin           GPIO_PIN_14
#define MDM_DTR_GPIO_Port     GPIOD
#define MDM_RI_Pin            GPIO_PIN_15
#define MDM_RI_GPIO_Port      GPIOD
  
/* GPS pins */
#define GNSS_EN_Pin           GPIO_PIN_8
#define GNSS_EN_GPIO_Port     GPIOC
#define GNSS_DRDY_Pin         GPIO_PIN_10
#define GNSS_DRDY_GPIO_Port   GPIOA


/* RS485 pins */
#define RS485_DE_Pin          GPIO_PIN_4
#define RS485_DE_GPIO_Port    GPIOD
#define RS485_TX_Pin          GPIO_PIN_5
#define RS485_TX_GPIO_Port    GPIOD
#define RS485_RX_Pin          GPIO_PIN_6
#define RS485_RX_GPIO_Port    GPIOD


/* WiFi pins */
#define WIFI_SPI_CS_Pin         GPIO_PIN_3
#define WIFI_SPI_CS_GPIO_Port   GPIOD
#define WIFI_INT_Pin            GPIO_PIN_2
#define WIFI_INT_GPIO_Port      GPIOD
#define WIFI_SCLK_Pin           GPIO_PIN_3
#define WIFI_SCLK_GPIO_Port     GPIOB
#define WIFI_SPI_DIN_Pin        GPIO_PIN_5
#define WIFI_SPI_DIN_GPIO_Port  GPIOB
#define WIFI_SPI_DOUT_Pin       GPIO_PIN_4
#define WIFI_SPI_DOUT_GPIO_Port GPIOB
#define WIFI_HIB_Pin            GPIO_PIN_6
#define WIFI_HIB_GPIO_Port      GPIOB
#define WIFI_RESET_Pin          GPIO_PIN_7
#define WIFI_RESET_GPIO_Port    GPIOB

  
/* I2C pins */
#define I2C1_SCL_Pin            GPIO_PIN_8
#define I2C1_SCL_GPIO_Port      GPIOB
#define I2C1_SDA_Pin            GPIO_PIN_9
#define I2C1_SDA_GPIO_Port      GPIOB
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/
   
typedef void (*bsp_button_callback_t)(uint32_t button, uint32_t event);     
    
/*------------------------- PUBLIC VARIABLES ---------------------------------*/

extern SPI_HandleTypeDef bsp_sd_spi_handle;

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/

void BSP_init(void);

void BSP_WD_Feed(uint32_t instance);

void BSP_Delay(uint32_t microseconds);

void BSP_Led_On(uint32_t id);
void BSP_Led_Off(uint32_t id);
void BSP_Led_Toggle(uint32_t id);
void BSP_Led_Pwm_Control(uint32_t id, uint32_t duty_cycle);

void BSP_Button_RegisterCallback(uint32_t button, bsp_button_callback_t callback);
uint32_t BSP_Button_GetPinState(uint32_t button);

uint32_t BSP_ResetReasonGet(void);

void BSP_PeriphReset(uint32_t target);


/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* _BSP_H */


