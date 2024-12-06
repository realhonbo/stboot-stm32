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
#include <string.h>
#include <cmsis_gcc.h>
#include <stdlib.h>
#include "bsp.h"

static UART_HandleTypeDef tty;
static char epb_buffer[EPB_BUF_SIZE];
static int early_flag = 1;

/* lib prototype */
int __io_putchar(int ch)
{
    HAL_UART_Transmit(&tty, (uint8_t *)&ch, 1, 100);
    return ch;
}

int __io_getchar(void)
{
    uint8_t ch;
    HAL_UART_Receive(&tty, &ch, 1, HAL_MAX_DELAY);
    return ch;
}

static float current(void)
{
    int load, val;
    float us, ms;

    load = SysTick->LOAD;
    val  = SysTick->VAL;

    us = (float)(load - val) / load * 0.001;
    ms = HAL_GetTick() * 0.001;

    return ms + us;
}

static void flush_epb(void)
{
    int i = 0;
    char *buf;

    while (epb_buffer[i]) {
        buf = &epb_buffer[i];
        printf("%s\r\n", buf);
        // jump over '\0'
        i += strlen(buf) + 1;
    }
}

/*
 * print info
 * gcc will inline current
 */
void pr_info(const char *fmt, ...)
{
    char *buf;
    char curr[15];
    static int epb_ptr;
    va_list args;

    // early print before console init
    if (early_flag) {
        sprintf(curr, "[%12.6f] ", current());

        va_start(args, fmt);
        vasiprintf(&buf, fmt, args);
        va_end(args);

        memcpy(&epb_buffer[epb_ptr], curr, sizeof(curr));
        epb_ptr += sizeof(curr);
        memcpy(&epb_buffer[epb_ptr], buf, strlen(buf));
        epb_ptr += strlen(buf);

        epb_buffer[epb_ptr++] = '\0';
        free(buf);
    } else {
        printf("[%12.6f] ", current());

        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);

        printf("\r\n");
    }
}

/*
 * uart hardware config
 */
void console_init(void)
{
    tty.Instance        = USART1;
    tty.Init.BaudRate   = UART_Baudrate;
    tty.Init.WordLength = UART_WORDLENGTH_8B;
    tty.Init.StopBits   = UART_STOPBITS_1;
    tty.Init.Parity = UART_PARITY_NONE;
    tty.Init.Mode   = UART_MODE_TX_RX;
    tty.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&tty);
    HAL_UARTEx_SetTxFifoThreshold(&tty, UART_TXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_SetRxFifoThreshold(&tty, UART_RXFIFO_THRESHOLD_1_8);
    HAL_UARTEx_DisableFifoMode(&tty);

#ifdef LOGO_GENERIC
    printf("\r\n");
    printf("-----------_______________  ____  ____  ______  \r\n");
    printf("   -------/ ___/_  __/ __ )/ __ \\/ __ \\/_  __/\r\n");
    printf("     -----\\__ \\ / / / __  / / / / / / / / /   \r\n");
    printf("   ------___/ // / / /_/ / /_/ / /_/ / / /      \r\n");
    printf("--------/____//_/ /_____/\\____/\\____/ /_/     \r\n");
    printf("\r\n");
#else
    printf("\r\n");
    printf("██████ ████████ ██████  ███████ ███████ ████████\r\n");
    printf("██        ██    ██   ██ ██   ██ ██   ██    ██   \r\n");
    printf("██████    ██    ██████  ██   ██ ██   ██    ██   \r\n");
    printf("    ██    ██    ██   ██ ██   ██ ██   ██    ██   \r\n");
    printf("██████    ██    ██████  ███████ ███████    ██   \r\n");
    printf("\r\n");
#endif

    early_flag = 0;
    flush_epb();
    pr_info("tty: uart1 init success");
}

void HAL_UART_MspInit(UART_HandleTypeDef *uartHandle)
{
    GPIO_InitTypeDef gpio;
    RCC_PeriphCLKInitTypeDef clk = {0};

    if (uartHandle->Instance == USART1) {
    /* freq */
        clk.PeriphClockSelection = RCC_PERIPHCLK_USART1;
        clk.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
        HAL_RCCEx_PeriphCLKConfig(&clk);
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
    /* pin: PA9 PA10 ---> TX RX */
        gpio.Pin  = GPIO_PIN_10 | GPIO_PIN_9;
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
        gpio.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &gpio);
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