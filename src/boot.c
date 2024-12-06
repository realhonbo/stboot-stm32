#include <stm32h7xx_hal.h>
#include <stdio.h>
#include <memory.h>
#include <cmsis_gcc.h>
#include "bsp.h"
#include "qspi_flash.h"
#include "errno.h"


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
    pr_info("vector: vectors -> DTCM 0x%08x, .itcm -> ITCM 0x%08x", &_isr_start, &_itcm_start);
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
static void kernel_entry(int kernel, int fdt)
{
    if(!kernel && !fdt) {
        kernel = KERNEL_ADDR;
        fdt  = FDT_ADDR;
    }
    pr_info("boot: kernel addr: 0x%x, fdt addr: 0x%x", kernel, fdt);
    pr_info("");
    pr_info("boot: ready to boot kernel ...");
    pr_info("");

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
    ::"r"(SysTick_BASE),"r"(fdt),"r"(kernel | 1)
    :"memory","cc","r0","r1","r2");
}

/**
 * console command
 * 
 ** if enter no-command, use default config in bsp.h
 * else use entered address
 *
 *@usage: boot    90010000 -   90000000
 *        boot  0x90010000 - 0x90000000
 *        <cmd>   <kernel> -    <fdt>
 *
 */
int console_cmd(void)
{
    int kernel = 0, fdt = 0;
    char buf[64] = "", cmd[8] = "";

#ifdef CONSOLE_CMD
    printf("st-boot > ");
    setvbuf(stdin, NULL, _IONBF, 0);

    command_read(buf);

    // solve command
    if (strlen(buf)) {
        if (parse_command(buf, cmd, &kernel, &fdt))
            return -EFAULT;
    }
#endif
    kernel_entry(kernel, fdt);
}


int main(void) {
    // config system clock
    HAL_Init();
    sysclk_config();
    ////remap_ivt_to_tcm();
    // mpu setup
    mpu_config();
    SCB_EnableICache();
    SCB_EnableDCache();

    // init console and led
    console_init();
    led_init();

    // set nor_flash and sdram
    QSPI_W25Qxx_Init();
    QSPI_W25Qxx_MMMode();
    sdram_init();

    // jump to kernel
    console_cmd();
}
