#ifndef BSP_EASYCONFIG_H
#define BSP_EASYCONFIG_H

/*
* * hardware configs
 */
#define SDRAM_BASE_ADDR         0xC0000000
#define SDRAM_SIZE_MB           32
#define FLASH_BASE_ADDR         0x08000000
#define QSPI_FLASH_BASE_ADDR    0x90000000

/*
 * FDT address:     0x9000_0000 - 0x9001_0000 : 64KB, start of qspi-flash
 * Kernel address:  0x9001_0000 -
 */
#define FDT_ADDR                QSPI_FLASH_BASE_ADDR
#define FDT_SIZE                0x10000
#define KERNEL_ADDR            (QSPI_FLASH_BASE_ADDR + FDT_SIZE)

#define UART_Baudrate           115200
#define LED_BLINK_TIME          10




void sysclk_config(void);
void mpu_config(void);
void sdram_init(void);
void led_init(void);
void led_timer_handler(void);
void uart1_tty_init(void);
void Error_Handler(char *file, int line);
void error_print(void);

#endif
