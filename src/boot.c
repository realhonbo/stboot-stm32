#include <stm32h7xx_hal.h>
#include <stdio.h>
#include <memory.h>
#include <cmsis_gcc.h>
#include "bsp.h"
#include "qspi_flash.h"
#include "setup.h"

////static struct tag *params;
/*
 * move vector_table from flash to DTCM
 * and fast instructions to ITCM
 */
static void remap_ivt_to_tcm(void)
{
    extern char _isr_start, _isr_size,
    _itcm_start, _itcm_size, _itcm_at_start;

    __disable_irq();

    memcpy(&_isr_start, (int *)FLASH_BASE_ADDR, (int)(&_isr_size));
    SCB->VTOR = (int)&_isr_start;
    memcpy(&_itcm_start, &_itcm_at_start, (int)(&_itcm_size));
    
    __enable_irq();
}

/**
 * jump to kernel

 * @b  reserved
 * @b  mach: match machine_id in linux: arch/arm/mach-
 *           if use DT, set ~0
 * @b  fdt: FDT address

 * (KERNEL_ADDR | 1) -> for thumb mode
 */
static void kernel_entry()
{
    printf("[ bootargs ]: kernel addr: %x, fdt addr: %x\r\n", KERNEL_ADDR, FDT_ADDR);
    printf("\r\n");
    printf("[ boot ]: succeed, ready to boot kernel... \r\n");
    printf("\r\n");
    // dcache should be closed before kernel init
    SCB_DisableDCache();
    ////asm volatile ( "ldr pc, =0x08020001" );
    void (*kernel)(__u32 reserved, __u32 mach, __u32 fdt) =
            ( void (*)(__u32, __u32, __u32))(KERNEL_ADDR | 1);
    kernel(0, ~0, FDT_ADDR);
}

int main(void) {
    ////remap_ivt_to_tcm();
    // init i/dcache
    mpu_config();
    SCB_EnableICache();
    SCB_EnableDCache();
    
    // config system clock
    HAL_Init();
    sysclk_config();

    // init tty and led
    uart1_tty_init();
    led_init();
    
    // set nor_flash, and xip to mm
    QSPI_W25Qxx_Init();
    QSPI_W25Qxx_MMMode();

    // config sdram
    sdram_init();

    // jump to kernel
    kernel_entry();
}
