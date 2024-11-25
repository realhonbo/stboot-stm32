/**
 * @file uart.c
 * @author Honbo (hehongbo918@gmail.com)
 * @brief ST-Boot console, also serves for linux earlyprintk
 * @version 1.0
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <stm32h7xx_hal.h>
#include <stdio.h>
#include <stdarg.h>
#include "bsp.h"

static UART_HandleTypeDef tty;

int __io_putchar(int ch)
{
    HAL_UART_Transmit(&tty, (uint8_t *)&ch, 1, 100);
    return ch;
}

void pr_info(const char *fmt, ...)
{
    float ts = HAL_GetTick() * 0.001;

    printf("[%12.6f] ", ts);

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\r\n");
}

void uart1_tty_init(void)
{
    tty.Instance        = USART1;
    tty.Init.BaudRate   = UART_Baudrate;
    tty.Init.WordLength = UART_WORDLENGTH_8B;
    tty.Init.StopBits   = UART_STOPBITS_1;
    tty.Init.Parity = UART_PARITY_NONE;
    tty.Init.Mode   = UART_MODE_TX_RX;
    tty.Init.HwFlowCtl      = UART_HWCONTROL_NONE;
    tty.Init.OverSampling   = UART_OVERSAMPLING_16;
    tty.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    tty.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    tty.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_UART_Init(&tty);
    HAL_UARTEx_SetTxFifoThreshold(&tty, UART_TXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_SetRxFifoThreshold(&tty, UART_RXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_DisableFifoMode(&tty);

    printf("\r\n");
    printf(" _____   _____   _____   _____   _____   _____    \r\n");
    printf("/  ___/ |_   _| |  _  \\ /  _  \\ /  _  \\ |_   _|\r\n");
    printf("| |___    | |   | |_| | | | | | | | | |   | |     \r\n");
    printf("\\___  \\   | |   |  _  | | | | | | | | |   | |   \r\n");
    printf(" ___| |   | |   | |_| | | |_| | | |_| |   | |     \r\n");
    printf("/_____/   |_|   |_____/ \\_____/ \\_____/   |_|   \r\n");
    printf("\r\n");

    pr_info("tty: uart1 init success");
}

void HAL_UART_MspInit(UART_HandleTypeDef *uartHandle)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    if (uartHandle->Instance == USART1) {
    /* freq */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1;
        PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
        HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
    /* pin: PA9 PA10 ---> TX RX */
        GPIO_InitStruct.Pin  = GPIO_PIN_10 | GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    /* interrupt */
        HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *uartHandle)
{
    if (uartHandle->Instance == USART1) {
        __HAL_RCC_USART1_CLK_DISABLE();
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_10 | GPIO_PIN_9);
        HAL_NVIC_DisableIRQ(USART1_IRQn);
    }
}