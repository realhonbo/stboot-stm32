<p align="center">
  <a>
    <img src=".vscode/.pic/stboot_v2.0.png"  width="716" height="375">
  </a>
</p>



> *stm32 bootloader for linux, base on hal library, easy for you to customize functions*
- **Boards Support**

  | Board                | MCU           | SDRAM        | QSPI-Flash | LCD                 |
  | -------------------- | ------------- | ------------ | ---------- | ------------------- |
  | FK743M5（Fank-Tech） | STM32H743XIH6 | W9825G6KH-6I | W25Q64     | ST7789V - 240 x 320 |
  |                      |               |              |            |                     |
  




- **Compile options configure**

  ```cmake
  5:  set(TOOLCHAIN_PATH B:/arm-gnu-toolchain ) #! toolchain location
  ```

  ```cmake
  44: add_definitions(-DUSE_HAL_DRIVER -DSTM32H743xx) #! H743
  ```



- **Board Config**
  
  - fcpu config：
  
    > **480MHz**（*BogoMIPS*  240）：`SYSCLK_PLL_N = 192`，`SYSCLK_PLL_M = 5`
    >
    > **550MHz**（*BogoMIPS*  275）：`SYSCLK_PLL_N = 88`，`SYSCLK_PLL_M = 2`
  
  - `EPB_BUF_SIZE`：size of buffer for early print （default to 512B）
  
  - `USE_SRAM_D2` `USE_SRAM_D3`：use SRAM in D2（288KB）and D3（64KB）
  
  - `FDT_ADDR` `FDT_SIZE`：the base address and size of fdt（default 64KB，a Flash Block）
  
  - `KERNEL_ADDR`：base address of kernel = `FDT_ADDR` + `FDT_SIZE`
  
  - `UART_Baudrate`：default **115200** bps
  
  - `CONSOLE_CMD`：whether use command console
  
  - `LED_BLINK_TIME`：the blink interval of LED（default **82ms**）
  
  
  
  
- **Shell**
  
  > add a new command
  
  ```c
  void help_newcmd(void) { ... }
  int do_newcmd(const char *buf) { ... }
  SHELL_EXPORT_CMD(newcmd, help_newcmd, do_newcmd);
  ```
  
  
  
- **Program**

  - `stboot.bin` -> `0x0800_0000`

  - `stm32h743i-disco.dtb.bin` -> `0x9000_0000`
  
  - `xipImage.bin` -> `0x9001_0000`
  
    > User Name:	**root**
    >
    > Passwd:		**0**
  
    ```shell
    ----------- _______________  ____  ____  ______  
       ------- / ___/_  __/ __ )/ __ \/ __ \/_  __/
          ---- \__ \ / / / __  / / / / / / / / /   
       ------ ___/ // / / /_/ / /_/ / /_/ / / /      
    -------- /____//_/ /_____/\____/\____/ /_/     
    
    [    0.000003] sysclk: system clock configured, CPU 480MHz
    [    0.000085] tcm: vectors -> dtcm 0x20000000, .itcm -> itcm 0x00001000
    [    0.000157] mpu: mem 0x24000000 setup, size 512KB
    [    0.000219] mpu: mem 0xc0000000 setup, size 32MB
    [    0.000281] mpu: mem 0x08000000 setup, size 2MB
    [    0.000341] mpu: mem 0x90000000 setup, size 8MB
    [    0.052661] tty: uart1 init success
    [    0.056168] led: gpioc-13 as triggered led
    [    0.060292] flash: w25q64 flash ( ID:EF4017 ) init success
    [    0.067001] sdram: configure success
    st-boot > boot 90010000 - 90000000
    [    0.070592] boot: kernel addr: 0x90010000, fdt addr: 0x90000000
    [    0.076539] 
    [    0.078035] boot: ready to boot kernel ...
    [    0.082149]
    
    ... ...(kernel booting message)
    
    ~ # uname -a
    Linux user 6.12.0 #3 Sun Nov 24 20:40:54 CST 2024 armv7ml GNU/Linux
    ~ # cat /proc/meminfo 
    MemTotal:          32012 kB
    MemFree:           23908 kB
    ... ...
    Percpu:               32 kB
    ~ # iostat 
    Linux 6.12.0 (user)     01/01/70        _armv7ml_       (1 CPU)
    
    avg-cpu:  %user   %nice %system %iowait  %steal   %idle
               0.61    0.00    5.99    0.10    0.00   93.30
    
    Device:            tps   Blk_read/s   Blk_wrtn/s   Blk_read   Blk_wrtn
    mmcblk0           1.81        14.77         6.65       1758        792
    
    ~ # 
    ```
    
    