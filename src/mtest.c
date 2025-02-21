/**
 * @file memory-speed.c
 * @brief cite to 安富莱 fmc-sdram test
 *        rw 32B at once
 * @version 0.8
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include <stm32h7xx_hal.h>
#include <stdio.h>
#include <memory.h>
#include <cmsis_gcc.h>
#include "bsp.h"
#include "qspi-flash.h"

#define RW_SIZE      (SDRAM_SIZE_MB * 1024 * 1024)
// winbond: cannot read the last bit at XIP mode
#define RD_SIZE      (QSPI_FLASH_SIZE_MB * 1024 * 1024 - 1)
#define SPEED(size)  (size / 1000 / (end_time - start_time))

static int start_time, end_time;

static inline void memory_write(__IO int *addr, int size)
{
    int i, j = 0;
    int _SIZE = size / sizeof(int) / 32;
    start_time = HAL_GetTick();
    for (i = 0; i < _SIZE; i++) {
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;

        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;

        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;

        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
        *addr++ = j++;
    }
    end_time = HAL_GetTick();
}

static inline void memory_read(__IO int *addr, int size)
{
    int i, tmp;
    int _SIZE = size / sizeof(int) / 32;
    start_time = HAL_GetTick();
    for (i = 0; i < _SIZE; i++) {
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;

        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;

        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;

        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
        tmp = *addr++;
    }
    end_time = HAL_GetTick();
}

/**
 * @brief 测试 SDRAM 读写速度 & QSPI-Flash 读速度
 */
__itcm void memory_speed_test(void)
{
    extern QSPI_HandleTypeDef hqspi;

/* SDRAM */
    memory_write((int *)SDRAM_BASE_ADDR, RW_SIZE);
    printf("sdram write: %d MB/s\r\n", SPEED(RW_SIZE));

    memory_read((int *)SDRAM_BASE_ADDR, RW_SIZE);
    printf("sdram  read: %d MB/s\r\n", SPEED(RW_SIZE));

/* QSPI Flash */
    QSPI_W25Qxx_MMMode();
    memory_read((int *)QSPI_FLASH_BASE_ADDR, RD_SIZE);
    printf("qspi-flash read(mm-mode): %d MB/s\r\n", SPEED(RD_SIZE));
    HAL_QSPI_Abort(&hqspi);
    QSPI_W25Qxx_Init();

    printf("\r\n");
}