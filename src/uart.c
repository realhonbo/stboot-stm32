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

#define SOH_RED     "\033[31m"
#define SOH_YEL     "\033[33m"
#define SOH_BLK     "\033[30m"
#define EOL_COLOR   "\033[0m"

static UART_HandleTypeDef huart;

struct epb {
    char buffer[EPB_BUF_SIZE];
    int ptr;
    int flag;
};
static struct epb ebuf = { .flag = 1 };


/* lib prototype */
int _write(int file, char *ptr, int len)
{
    (void)file;
    HAL_UART_Transmit(&huart, (uint8_t *)ptr, len, 0xff);
    return len;
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    HAL_UART_Receive(&huart, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

static float get_log_time(void)
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
 * message before uart init
 *   cached into ebp buffer
 * else
 *   print directly
 *
 * gcc will inline get_log_time
 */
static int get_log_level(const char *level_str) {
    const char *start = strchr(level_str, '<');
    if (!start || start[1] > '7' || start[1] < '0' || start[2] != '>')
        return 0; // invalid, return default
    return start[1] - '0';
}

static void log_prefix(int level)
{
    char *prefix;

    if (ebuf.flag == 0)
        switch (level) {
        case 3: printf(SOH_RED "err: "   EOL_COLOR); break;
        case 4: printf(SOH_YEL "warn: "  EOL_COLOR); break;
        case 7: printf(SOH_BLK "DEBUG: " EOL_COLOR); break;
        }
    else {
        switch (level) {
        case 3: prefix = SOH_RED "err: "   EOL_COLOR; break;
        case 4: prefix = SOH_YEL "warn: "  EOL_COLOR; break;
        case 7: prefix = SOH_BLK "DEBUG: " EOL_COLOR; break;
        default: return;
        }
        memcpy(&ebuf.buffer[ebuf.ptr], prefix, strlen(prefix));
        ebuf.ptr += strlen(prefix);
    }
}

void printk(const char *fmt, ...)
{
    char log_time[15];
    char *buf;
    va_list args;
    int level = get_log_level(fmt);

    va_start(args, fmt);

    if (likely(ebuf.flag == 0)) {
        if (level) {
            printf("[%12.6f] ", get_log_time());
            log_prefix(level);
            fmt += 3;
        }
        vprintf(fmt, args);
        printf("\r\n");
        goto end;
    }

    // early print
    if (level) {
        sprintf(log_time, "[%12.6f] ", get_log_time());
        fmt += 3;
    }
    vasiprintf(&buf, fmt, args);

    memcpy(&ebuf.buffer[ebuf.ptr],
            log_time, sizeof(log_time));
    ebuf.ptr += sizeof(log_time);

    log_prefix(level);

    memcpy(&ebuf.buffer[ebuf.ptr],
            buf, strlen(buf));
    ebuf.ptr += strlen(buf);

    ebuf.buffer[ebuf.ptr++] = '\0';
    free(buf);

end:
    va_end(args);
}

/*
 * uart hardware config
 */
void console_init(void)
{
    GPIO_InitTypeDef IO_Init;
    RCC_PeriphCLKInitTypeDef CLK_Init = {0};

    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    CLK_Init.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    CLK_Init.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
    HAL_RCCEx_PeriphCLKConfig(&CLK_Init);

    IO_Init.Pin  = GPIO_PIN_10 | GPIO_PIN_9;
    IO_Init.Mode = GPIO_MODE_AF_PP;
    IO_Init.Pull = GPIO_NOPULL;
    IO_Init.Speed = GPIO_SPEED_FREQ_HIGH;
    IO_Init.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &IO_Init);

    huart.Instance        = USART1;
    huart.Init.BaudRate   = UART_Baudrate;
    huart.Init.WordLength = UART_WORDLENGTH_8B;
    huart.Init.StopBits   = UART_STOPBITS_1;
    huart.Init.Parity = UART_PARITY_NONE;
    huart.Init.Mode   = UART_MODE_TX_RX;
    HAL_UART_Init(&huart);
    ebuf.flag = 0;

    printk("");
    printk("----------- _______________  ____  ____  ______  ");
    printk("   ------- / ___/_  __/ __ )/ __ \\/ __ \\/_  __/");
    printk("       --- \\__ \\ / / / __  / / / / / / / / /   ");
    printk("   ------ ___/ // / / /_/ / /_/ / /_/ / / /      ");
    printk("-------- /____//_/ /_____/\\____/\\____/ /_/     ");
    printk("");
    flush_epb();

    printk(KERN_INFO "console: uart1 init success");
}