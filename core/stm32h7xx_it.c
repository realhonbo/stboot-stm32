/**
  ******************************************************************************
  * @file    stm32h7xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
#include "stm32h7xx_hal.h"
#include "stm32h7xx_it.h"
#include <stdio.h>
#include "bsp.h"
/*
 *      Cortex Processor Interruption and Exception Handlers
 */
void SysTick_Handler(void)
{
        HAL_IncTick();
}

void HardFault_Handler(void)
{
        error_print();
        while ( 1 );
}

void SVC_Handler(void)
{

}

void PendSV_Handler(void)
{

}

/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
        while (1) {

        }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
        while (1) {

        }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
        while (1) {

        }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
        while (1) {

        }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{

}

/*
 * STM32H7xx Peripheral Interrupt Handlers
 * Add here the Interrupt Handlers for the used peripherals
 * For the available peripheral interrupt handler names,
 * please refer to the startup file (startup_stm32h7xx.s)
 */

/**
  * @brief This function handles USART1 global interrupt.
  */


/*
 * @brief: After Tx is completed, the SPI DMA should be stopped for next transfer
*/

