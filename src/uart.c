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

struct epb {
    char buffer[EPB_BUF_SIZE];
    int ptr;
    int flag;
};

static UART_HandleTypeDef tty;
static struct epb ebuf = {.flag = 1};


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

    while (ebuf.buffer[i]) {
        buf = &ebuf.buffer[i];
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
    char curr[15];
    char *buf;
    va_list args;

    va_start(args, fmt);

    if (likely(!ebuf.flag)) {
        printf("[%12.6f] ", current());
        vprintf(fmt, args);
        printf("\r\n");
    } else {
        // early print before console init
        sprintf(curr, "[%12.6f] ", current());
        vasiprintf(&buf, fmt, args);

        memcpy(&ebuf.buffer[ebuf.ptr],
                curr, sizeof(curr));
        ebuf.ptr += sizeof(curr);

        memcpy(&ebuf.buffer[ebuf.ptr],
                buf, strlen(buf));
        ebuf.ptr += strlen(buf);

        ebuf.buffer[ebuf.ptr++] = '\0';
        free(buf);
    }

    va_end(args);
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
    HAL_UARTEx_SetTxFifoThreshold(&tty, 0);
    HAL_UARTEx_SetRxFifoThreshold(&tty, 0);
    HAL_UARTEx_DisableFifoMode(&tty);

    printf("\r\n");
    printf("----------- _______________  ____  ____  ______  \r\n");
    printf("   ------- / ___/_  __/ __ )/ __ \\/ __ \\/_  __/\r\n");
    printf("      ---- \\__ \\ / / / __  / / / / / / / / /   \r\n");
    printf("   ------ ___/ // / / /_/ / /_/ / /_/ / / /      \r\n");
    printf("-------- /____//_/ /_____/\\____/\\____/ /_/     \r\n");
    printf("\r\n");

    ebuf.flag = 0;
    flush_epb();
    pr_info("tty: uart1 init success");
}

/**
 * PA9 ->TX     PA10 -> RX
 */
void HAL_UART_MspInit(UART_HandleTypeDef *uh)
{
    GPIO_InitTypeDef gpio;
    RCC_PeriphCLKInitTypeDef clk = {0};

    if (uh->Instance == USART1) {
        clk.PeriphClockSelection = RCC_PERIPHCLK_USART1;
        clk.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
        HAL_RCCEx_PeriphCLKConfig(&clk);
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        gpio.Pin  = GPIO_PIN_10 | GPIO_PIN_9;
        gpio.Mode = GPIO_MODE_AF_PP;
        gpio.Pull = GPIO_NOPULL;
        gpio.Speed = GPIO_SPEED_FREQ_LOW;
        gpio.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &gpio);
    }
}