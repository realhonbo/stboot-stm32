/**
 * @file bsp.c
 * @author Honbo (hehongbo918@gmail.com)
 * @brief mpu and clock config
 * @version 1.0
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <stm32h7xx_hal.h>
#include <stdio.h>
#include <string.h>
#include "bsp.h"

/*
 * write back + fetch on write
 */
void mpu_config(void)
{
        MPU_Region_InitTypeDef mpu_init = {0};

        HAL_MPU_Disable();
/* SRAM_D1 */
        mpu_init.Enable = MPU_REGION_ENABLE;
        mpu_init.BaseAddress = 0x24000000;
        mpu_init.Size = MPU_REGION_SIZE_512KB;
        mpu_init.SubRegionDisable = 0x0;
        mpu_init.AccessPermission = MPU_REGION_FULL_ACCESS;
        mpu_init.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
        mpu_init.IsBufferable = MPU_ACCESS_BUFFERABLE;
        mpu_init.IsCacheable = MPU_ACCESS_CACHEABLE;
        mpu_init.Number = MPU_REGION_NUMBER0;
        mpu_init.TypeExtField = MPU_TEX_LEVEL1;
        mpu_init.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
        HAL_MPU_ConfigRegion(&mpu_init);
        pr_info("mpu: mem 0x%08x setup, size 512KB", mpu_init.BaseAddress);
#ifdef USE_SRAM_D2 /* SRAM_D2 */
        mpu_init.BaseAddress = 0x30000000;
        mpu_init.Size = MPU_REGION_SIZE_256KB;
        mpu_init.Number = MPU_REGION_NUMBER1;
        HAL_MPU_ConfigRegion(&mpu_init);
        pr_info("mpu: mem 0x%08x setup, size 256KB", mpu_init.BaseAddress);
#endif
#ifdef USE_SRAM_D3 /* SRAM_D3 */
        mpu_init.BaseAddress = 0x38000000;
        mpu_init.Size = MPU_REGION_SIZE_64KB;
        mpu_init.Number = MPU_REGION_NUMBER2;
        HAL_MPU_ConfigRegion(&mpu_init);
        pr_info("mpu: mem 0x%08x setup, size 64KB", mpu_init.BaseAddress);
#endif
/* SDRAM */
        mpu_init.BaseAddress = SDRAM_BASE_ADDR;
        mpu_init.Size = MPU_REGION_SIZE_32MB;
        mpu_init.Number = MPU_REGION_NUMBER3;
        HAL_MPU_ConfigRegion(&mpu_init);
        pr_info("mpu: mem 0x%08x setup, size 32MB", mpu_init.BaseAddress);
/* FLASH */
        mpu_init.BaseAddress = FLASH_BASE_ADDR;
        mpu_init.Size = MPU_REGION_SIZE_2MB;
        mpu_init.Number = MPU_REGION_NUMBER4;
        HAL_MPU_ConfigRegion(&mpu_init);
        pr_info("mpu: mem 0x%08x setup, size 2MB", mpu_init.BaseAddress);
/* QSPI FLASH */
        mpu_init.BaseAddress = QSPI_FLASH_BASE_ADDR;
        mpu_init.Size = MPU_REGION_SIZE_8MB;
        mpu_init.Number = MPU_REGION_NUMBER5;
        HAL_MPU_ConfigRegion(&mpu_init);
        HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
        pr_info("mpu: mem 0x%08x setup, size 8MB", mpu_init.BaseAddress);
}


/**
 * 初始化系统时钟: MHz
 *       PLL_IN  = OSC_IN -> HSE  = 25
 *       Vco_IN  = PLL_IN / PLL_M = 5
 *       Vco_OUT = Vco_IN * PLL_N = 960
 *CPU:   FCLK = PLL_OUT = Vco_OUT / PLL_P = 480
 *USB:   1: PLL = Vco_OUT / PLL_Q  = 48
 *       2: HSI48
 *HCLK:  (AXI & AHB)  = 240
 *AHB:   Prescaler = 2
 *APB:  
 *       APB1 (D2) = 100
 *       APB2 (D2) = 100
 *       APB3 (D1) = 100
 *       APB4 (D3) = 100
 *FLASH: Latency (WS) = 4
 */
void sysclk_config(void)
{
        int fcpu;
        RCC_OscInitTypeDef RCC_OscInitStruct = {0};
        RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
        RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

/* 请求更新电源配置 */
        HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
/* 内部稳压器输出电压 */
        __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
        while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY));
        __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);
/* RCC Oscillators振荡器 */
        RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE |
                                           RCC_OSCILLATORTYPE_HSI48;
        RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
        RCC_OscInitStruct.HSI48State     = RCC_HSI48_ON;
        RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
        RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
        RCC_OscInitStruct.PLL.PLLM       = SYSCLK_PLL_M;
        RCC_OscInitStruct.PLL.PLLN       = SYSCLK_PLL_N;
        RCC_OscInitStruct.PLL.PLLP       = SYSCLK_PLL_P;
        RCC_OscInitStruct.PLL.PLLQ       = 4; // 2
        RCC_OscInitStruct.PLL.PLLR       = 2;
        RCC_OscInitStruct.PLL.PLLRGE     = RCC_PLL1VCIRANGE_2;
        RCC_OscInitStruct.PLL.PLLVCOSEL  = RCC_PLL1VCOWIDE;
        RCC_OscInitStruct.PLL.PLLFRACN   = 0;
        if (HAL_RCC_OscConfig(&RCC_OscInitStruct)) {
                pr_info("Error: file: %s, line: %d", 
                                __FILE__, __LINE__);
        }
/* 初始化CPU | AHB | APB总线时钟 */
        RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | 
                                      RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | 
                                      RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
        RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
        RCC_ClkInitStruct.SYSCLKDivider  = RCC_SYSCLK_DIV1;
        RCC_ClkInitStruct.AHBCLKDivider  = RCC_HCLK_DIV2;
        RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
        RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
        RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
        RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
        if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4)) {
                pr_info("Error: file: %s, line: %d", 
                                __FILE__, __LINE__);
        }

        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB | RCC_PERIPHCLK_SDMMC |
                                                   RCC_PERIPHCLK_QSPI;
        PeriphClkInitStruct.UsbClockSelection    = RCC_USBCLKSOURCE_HSI48;
        PeriphClkInitStruct.SdmmcClockSelection  = RCC_SDMMCCLKSOURCE_PLL;
        PeriphClkInitStruct.QspiClockSelection   = RCC_QSPICLKSOURCE_D1HCLK;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct)) {
                pr_info("Error: file: %s, line: %d", 
                                __FILE__, __LINE__);
        }
/* 使用IO高速模式, 要使能IO补偿
 * 1. CSI clock
 * 2. SYSCFG clock
 * 3. 使能 IO 补偿单元, 设置 SYSCFG_CCCSR 寄存器的 bit-0
 * 4. USBFS clock
 */     __HAL_RCC_CSI_ENABLE();
        __HAL_RCC_SYSCFG_CLK_ENABLE();
        __HAL_RCC_USB_OTG_FS_CLK_ENABLE();
        HAL_EnableCompensationCell();
/* D2、D3域的SRAM需要单独使能 */
#if defined(USE_SRAM_D2) || defined(USE_SRAM_D3)
        __HAL_RCC_D2SRAM1_CLK_ENABLE();
#endif
        fcpu = HSE_FREQUENCY * SYSCLK_PLL_N / SYSCLK_PLL_M / SYSCLK_PLL_P;
        pr_info("sysclk: system clock configured, CPU %dMHz", fcpu);
}

/**
 * @usage __FILE__ __LINE__ : 获取当前文件名称和代码行号
 */
void Error_Handler(char *file, int line)
{
        pr_info("Error: file: %s, line: %d", file, line);
}


