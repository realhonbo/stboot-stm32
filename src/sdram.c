/**
 * @file sdram.c
 * @author Honbo (hehongbo918@gmail.com)
 * @brief Winbond  W9825C6KH-6I
 * @version 1.0
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <stm32h7xx_hal.h>
#include <stdio.h>
#include "bsp.h"


#define SDRAM_TIMEOUT                ((uint32_t)0x1000)
// burst length
#define SDRAM_MRD_BL_1               ((uint16_t)0x0000)
#define SDRAM_MRD_BL_2               ((uint16_t)0x0001)
#define SDRAM_MRD_BL_4               ((uint16_t)0x0002)
#define SDRAM_MRD_BL_8               ((uint16_t)0x0004)
// burst type
#define SDRAM_MRD_BT_SEQUENTIAL      ((uint16_t)0x0000)
#define SDRAM_MRD_BT_INTERLEAVED     ((uint16_t)0x0008)
// CAS latency - stable in latency_3
#define SDRAM_MRD_CAS_LATENCY_2      ((uint16_t)0x0020)
#define SDRAM_MRD_CAS_LATENCY_3      ((uint16_t)0x0030)
// write burst mode
#define SDRAM_MRD_WB_MODE_PROGRAMMED ((uint16_t)0x0000)
#define SDRAM_MRD_WB_MODE_SINGLE     ((uint16_t)0x0200)
// op mode默认为正常模式

/**
 * 初始化fmc-sdram引脚
 *     PF0  ---> FMC_A0        PD14 ---> FMC_D0       PC0  ---> FMC_SDNWE
 *     PF1  ---> FMC_A1        PD15 ---> FMC_D1       PG15 ---> FMC_SDNCAS
 *     PF2  ---> FMC_A2        PD0  ---> FMC_D2       PF11 ---> FMC_SDNRAS
 *     PF3  ---> FMC_A3        PD1  ---> FMC_D3       PH3  ---> FMC_SDNE0
 *     PF4  ---> FMC_A4        PE7  ---> FMC_D4       PG4  ---> FMC_BA0
 *     PF5  ---> FMC_A5        PE8  ---> FMC_D5       PG5  ---> FMC_BA1
 *     PF12 ---> FMC_A6        PE9  ---> FMC_D6       PH2  ---> FMC_SDCKE0
 *     PF13 ---> FMC_A7        PE10 ---> FMC_D7       PG8  ---> FMC_SDCLK
 *     PF14 ---> FMC_A8        PE11 ---> FMC_D8       PE1  ---> FMC_NBL1
 *     PF15 ---> FMC_A9        PE12 ---> FMC_D9       PE0  ---> FMC_NBL0
 *     PG0  ---> FMC_A10       PE13 ---> FMC_D10
 *     PG1  ---> FMC_A11       PE14 ---> FMC_D11
 *     PG2  ---> FMC_A12       PE15 ---> FMC_D12
 *                             PD8  ---> FMC_D13
 *                             PD9  ---> FMC_D14
 *                             PD10 ---> FMC_D15
 */
void HAL_SDRAM_MspInit(SDRAM_HandleTypeDef *hsdram)
{
        GPIO_InitTypeDef gpio_init;

        __HAL_RCC_FMC_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();
        __HAL_RCC_GPIOE_CLK_ENABLE();
        __HAL_RCC_GPIOF_CLK_ENABLE();
        __HAL_RCC_GPIOG_CLK_ENABLE();
        __HAL_RCC_GPIOH_CLK_ENABLE();

        gpio_init.Mode = GPIO_MODE_AF_PP;
        gpio_init.Pull = GPIO_NOPULL;
        gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init.Alternate = GPIO_AF12_FMC;
        gpio_init.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                        GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 |
                        GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
        HAL_GPIO_Init(GPIOF, &gpio_init);

        gpio_init.Pin = GPIO_PIN_2 | GPIO_PIN_3;
        HAL_GPIO_Init(GPIOH, &gpio_init);

        gpio_init.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 |
                        GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15;
        HAL_GPIO_Init(GPIOG, &gpio_init);

        gpio_init.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
                        GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |
                        GPIO_PIN_15 | GPIO_PIN_0 | GPIO_PIN_1;
        HAL_GPIO_Init(GPIOE, &gpio_init);

        gpio_init.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 |
                        GPIO_PIN_15 | GPIO_PIN_0 | GPIO_PIN_1;
        HAL_GPIO_Init(GPIOD, &gpio_init);

        gpio_init.Pin = GPIO_PIN_0;
        HAL_GPIO_Init(GPIOC, &gpio_init);
}

/**
 * SDRAM相关时序和控制方式配置 - SDCMR
 */
static void sdram_send_command(SDRAM_HandleTypeDef *hsdram)
{
        FMC_SDRAM_CommandTypeDef cmd;
/* 时钟使能 */
        cmd.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
        cmd.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
        cmd.AutoRefreshNumber = 1;
        cmd.ModeRegisterDefinition = 0;
        HAL_SDRAM_SendCommand(hsdram, &cmd, SDRAM_TIMEOUT);
        HAL_Delay(1); // 插入至少100us的延迟
/* 预充电 */
        cmd.CommandMode = FMC_SDRAM_CMD_PALL;
        HAL_SDRAM_SendCommand(hsdram, &cmd, SDRAM_TIMEOUT);
/* 自动刷新 */
        cmd.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
        cmd.AutoRefreshNumber = 8;
        HAL_SDRAM_SendCommand(hsdram, &cmd, SDRAM_TIMEOUT);
/* 加载模式 */
        cmd.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
        cmd.AutoRefreshNumber = 1;
        cmd.ModeRegisterDefinition = SDRAM_MRD_BL_2 | SDRAM_MRD_BT_SEQUENTIAL |
                SDRAM_MRD_CAS_LATENCY_3 | SDRAM_MRD_WB_MODE_SINGLE;
        HAL_SDRAM_SendCommand(hsdram, &cmd, SDRAM_TIMEOUT);
}

/* 初始化FMC和SDRAM配置
 *
 * 2分频-100MHz | 开启突发传输 | 读延迟-实际测此位可以设置无需延迟
 *
 * 每行刷新所需时钟数
 * ! 如果在接受读取请求时发生内部刷新请求
 *   则刷新率必须增加20个sdram时钟周期以获得安全余量
 * ! 目前公认的sdram中电容保存数据的上限是64ms, 即刷新周期64ms
 *
 * 刷新周期 / 行数 * 时钟速度 – 20 = 64ms / 8192 * 120MHz (or 1.2 x 10^5/ms) - 20 = 917.5
 */
void sdram_init(void)
{
        FMC_SDRAM_TimingTypeDef timing;
        SDRAM_HandleTypeDef hsdram1;

        hsdram1.Instance    = FMC_SDRAM_DEVICE;
        hsdram1.Init.SDBank = FMC_SDRAM_BANK1;
        hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_1;
        hsdram1.Init.ReadBurst     = FMC_SDRAM_RBURST_ENABLE;
        hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
        hsdram1.Init.WriteProtection    = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
        hsdram1.Init.CASLatency         = FMC_SDRAM_CAS_LATENCY_3;
        hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
        hsdram1.Init.MemoryDataWidth    = FMC_SDRAM_MEM_BUS_WIDTH_16;
        hsdram1.Init.RowBitsNumber      = FMC_SDRAM_ROW_BITS_NUM_13;
        hsdram1.Init.ColumnBitsNumber   = FMC_SDRAM_COLUMN_BITS_NUM_9;
/* FMC使用的HCLK3时钟，200MHz，用于SDRAM的话，至少2分频，也就是100MHz，即1个SDRAM时钟周期是10ns
 * 下面参数单位均为10ns
 */     timing.LoadToActiveDelay    = 2; // 20ns, TMRD定义加载模式寄存器的命令与激活命令或刷新命令之间的延迟
        timing.ExitSelfRefreshDelay = 7; // 70ns, TXSR定义从发出自刷新命令到发出激活命令之间的延迟
        timing.SelfRefreshTime      = 4; // 50ns, TRAS定义最短的自刷新周期
        timing.RowCycleDelay        = 7; // 70ns, TRC定义刷新命令和激活命令之间的延迟
        timing.WriteRecoveryTime    = 3; // 30ns, TWR定义在写命令和预充电命令之间的延迟
        timing.RPDelay              = 2; // 20ns, TRP定义预充电命令与其它命令之间的延迟
        timing.RCDDelay             = 2; // 20ns, TRCD定义激活命令与读/写命令之间的延迟
/* 初始化FMC接口
 * 配置命令和刷新率
 */     if (HAL_SDRAM_Init(&hsdram1, &timing))
            printk(KERN_ERR "sdram: init failed");

        sdram_send_command(&hsdram1);

        if (HAL_SDRAM_ProgramRefreshRate(&hsdram1, 918))
            printk(KERN_INFO "sdram: chuck in set refresh rate");

        printk(KERN_INFO "sdram: configure success");
}

