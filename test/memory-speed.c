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
#include "qspi_flash.h"

#define RW_SIZE         0x400000    // 测试数据量: Byte
#define CaculateSpeed   (RW_SIZE / 1000 / (end_time - start_time))

__IO int *sdram = (int *)SDRAM_BASE_ADDR;
__IO int *qspi  = (int *)QSPI_FLASH_BASE_ADDR;
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
void memory_speed_test(void) {
    pr_info("");
/* SDRAM */
    SCB_DisableDCache();
    memory_write(sdram);
    pr_info("关闭Cache: SDRAM 写速度: %d MB/s", CaculateSpeed);
    memory_read(sdram);
    pr_info("关闭Cache: SDRAM 读速度: %d MB/s", CaculateSpeed);

    SCB_EnableDCache();
    memory_write(sdram);
    pr_info("开启Cache: SDRAM 写速度: %d MB/s", CaculateSpeed);
    memory_read(sdram);
    pr_info("开启Cache: SDRAM 读速度: %d MB/s", CaculateSpeed);

/* QSPI Flash */
    SCB_DisableDCache();
    memory_read(qspi);
    pr_info("关闭Cache: QSPI Flash读速度: %d MB/s", CaculateSpeed);
    
    SCB_EnableDCache();
    memory_read(qspi);
    pr_info("开启Cache: QSPI Flash读速度: %d MB/s", CaculateSpeed);

    while ( 1 );
}