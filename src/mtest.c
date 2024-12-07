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

#define RW_SIZE         0x400000    // 测试数据量: Byte
#define caculate_speed   (RW_SIZE / 1000 / (end_time - start_time))

static int start_time, end_time;

static inline void memory_write(__IO int *addr)
{
    int i, j = 0;
    int _SIZE = RW_SIZE / sizeof(int) / 32;
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

static inline void memory_read(__IO int *addr)
{
    int i, tmp;
    int _SIZE = RW_SIZE / sizeof(int) / 32;
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
__itcm void memory_speed_test(void) {
    __IO int *sdram = (int *)SDRAM_BASE_ADDR;
    __IO int *qspi  = (int *)QSPI_FLASH_BASE_ADDR;

/* SDRAM */
    memory_write(sdram);
    printf("sdram write: %d MB/s\r\n", caculate_speed);

    memory_read(sdram);
    printf("sdram  read: %d MB/s\r\n", caculate_speed);

/* QSPI Flash */
    memory_read(qspi);
    printf("qspi-flash read: %d MB/s\r\n", caculate_speed);

    printf("\r\n");
}