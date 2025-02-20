#ifndef BSP_EASYCONFIG_H
#define BSP_EASYCONFIG_H

#define STBOOT_VERSION         "2.5.0 (develop)"

#define HSE_FREQUENCY          25

/*
 * CPU frequency
 */
#define SYSCLK_PLL_N           192
#define SYSCLK_PLL_M           5
#define SYSCLK_PLL_P           2

/*
 * early print buffer config
 */
#define EPB_BUF_SIZE           512

/*
 * hardware configs
 */
// USE_SRAM_D2 is not set
// USE_SRAM_D3 is not set
#define SDRAM_BASE_ADDR         0xC0000000
#define SDRAM_SIZE_MB           32
#define FLASH_BASE_ADDR         0x08000000
#define QSPI_FLASH_BASE_ADDR    0x90000000
#define QSPI_FLASH_SIZE_MB      8

/*
 * FDT address:     0x9000_0000 - 0x9001_0000 : 64KB, start of qspi-flash
 * Kernel address:  0x9001_0000 -
 */
#define FDT_ADDR                QSPI_FLASH_BASE_ADDR
#define FDT_SIZE                0x10000
#define KERNEL_ADDR            (QSPI_FLASH_BASE_ADDR + FDT_SIZE)

#define UART_Baudrate           115200
#define CONSOLE_CMD
#define LED_BLINK_TIME          82




#define __itcm      __attribute__((section(".itcm")))
#define noinline    __attribute__((noinline))
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

void sysclk_config(void);
void mpu_config(void);
void sdram_init(void);
void sdmmc_mount(void);
int  sdmmc_read_file(const char *, unsigned char **, int *);
void led_init(void);
void led_timer_handler(void);

void Error_Handler(char *, int);
void console_init(void);
void error_print(void);
void pr_info(const char *, ...);
void console_cmd(void);

void kernel_entry(int, int);

void memory_speed_test(void);

#endif
