#include <stm32h7xx_hal.h>
#include <stdio.h>
#include <memory.h>
#include <cmsis_gcc.h>
#include "bsp.h"
#include "qspi_flash.h"
#include "setup.h"

#define TEST_SIZE   0x100000
#define ITERATIONS  10

void sdram_speed_test(void) {
    volatile int *sdram = (int *)SDRAM_BASE_ADDR;
    int i, j;
    int start_time, end_time;
    volatile int tmp;

    printf("\n");
/**
 * @brief 关闭DCache
 * 
 */
    SCB_DisableDCache();
    // 写
    start_time = HAL_GetTick();
    for (j = 0; j < ITERATIONS; j++) {
        for (i = 0; i < (TEST_SIZE / sizeof(int)); i++) {
            sdram[i] = i;
        }
    }
    end_time = HAL_GetTick();
    printf("[关闭Cache] SDRAM写速度: %.2f MB/s\r\n",
           ((float)(TEST_SIZE * ITERATIONS) / (end_time - start_time)) / 1000);
    // 读
    start_time = HAL_GetTick();
    for (j = 0; j < ITERATIONS; j++) {
        for (i = 0; i < (TEST_SIZE / sizeof(int)); i++) {
            tmp = sdram[i];
        }
    }
    end_time = HAL_GetTick();
    printf("[关闭Cache] SDRAM读速度: %.2f MB/s\r\n",
           ((float)(TEST_SIZE * ITERATIONS) / (end_time - start_time)) / 1000);

    SCB_EnableDCache();
    // 写
    start_time = HAL_GetTick();
    for (j = 0; j < ITERATIONS; j++) {
        for (i = 0; i < (TEST_SIZE / sizeof(int)); i++) {
            sdram[i] = i;
        }
    }
    end_time = HAL_GetTick();
    printf("[开启Cache] SDRAM写速度: %.2f MB/s\r\n",
           ((float)(TEST_SIZE * ITERATIONS) / (end_time - start_time)) / 1000);
    // 读
    start_time = HAL_GetTick();
    for (j = 0; j < ITERATIONS; j++) {
        for (i = 0; i < (TEST_SIZE / sizeof(int)); i++) {
            tmp = sdram[i];
        }
    }
    end_time = HAL_GetTick();
    printf("[开启Cache] SDRAM读速度: %.2f MB/s\r\n",
           ((float)(TEST_SIZE * ITERATIONS) / (end_time - start_time)) / 1000);

    while ( 1 );
}