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
 ** kernel(0, ~0, FDT_ADDR);
 *
 * void (*kernel)(__u32 reserved, __u32 mach, __u32 fdt) =
            ( void (*)(__u32, __u32, __u32))(KERNEL_ADDR | 1);
 * @b  reserved
 * @b  mach: match machine_id in linux: arch/arm/mach-
 *           if use DT, set ~0
 * @b  fdt: FDT address

 * (KERNEL_ADDR | 1) -> for thumb mode
 */
static void kernel_entry()
{
    printf("[ bootargs ]: kernel addr: 0x%x, fdt addr: 0x%x\r\n", KERNEL_ADDR, FDT_ADDR);
    printf("\r\n");
    printf("[ boot ]: st-boot took %dms, ready to boot kernel... \r\n", HAL_GetTick());
    printf("\r\n");
    // dcache should be closed before kernel init
    SCB_DisableDCache();
    asm volatile ( "ldr r0, [%0]\n"
    "bic r0, r0, #0x3\n"
    "str r0, [%0]\n" // close systick irq
    "cpsid i\n"
    "mov r0, #0\n"
    "mvn r1, #0\n"
    "mov r2, %1\n"
    "bx %2"
    ::"r"(SysTick_BASE),"r"(FDT_ADDR), "r"(KERNEL_ADDR | 1)
    :"memory","cc","r0","r1","r2");
}

int main(void) {
    // init i/dcache
    // config system clock
    mpu_config();
    SCB_EnableICache();
    SCB_EnableDCache();
    HAL_Init();
    sysclk_config();

    // init tty and led
    uart1_tty_init();
    led_init();
    
    // set nor_flash and sdram
    QSPI_W25Qxx_Init();
    QSPI_W25Qxx_MMMode();
    sdram_init();

    // jump to kernel
    kernel_entry();
}
