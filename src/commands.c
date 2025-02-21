/**
 * @file commands.c
 * @author Honbo (hehongbo918@gmail.com)
 * @brief provide commands for user console
 * @version 1.0
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <stm32h7xx_hal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bsp.h"
#include "errno.h"
#include "cmd.h"
#include "qspi-flash.h"
#include "ff.h"

extern struct cmd *head;

/*
 * parse number of `boot`
 */
static void __parse_number(const char *buf, int *idx, int *number, int hex)
{
    int i = *idx;
    *number = 0;

    if (buf[i] == '-') i++;

    if (hex) {
        if (!strncmp(&buf[i], "0x", 2)) i += 2;
        while (buf[i] != ' ' && buf[i] != '\0') {
            if (buf[i] >= '0' && buf[i] <= '9')
                *number = (*number * 16) + (buf[i] - '0');
            else if (buf[i] >= 'a' && buf[i] <= 'f')
                *number = (*number * 16) + (buf[i] - 'a' + 10);
            else if (buf[i] >= 'A' && buf[i] <= 'F')
                *number = (*number * 16) + (buf[i] - 'A' + 10);
            else
                break; // non-hex character, break out of the loop
            i++;
        }
    } else {
        while (buf[i] != ' ' && buf[i] != '\0') {
            if (buf[i] < '0' || buf[i] > '9')
                break;
            *number = (*number * 10) + (buf[i] - '0');
            i++;
        }
    }
    *idx = i;
}

/**
 * instead of: sscanf(buf, "%s %x - %x", cmd, &kernel, &fdt) != 3
 */
static int parse_address(const char *buf, int *kernel, int *fdt) {
    int idx = 0;
    *kernel = 0;
    *fdt = 0;

    if (strlen(buf) < 9)
        return 0;

    // jump over name
    while (buf[idx] != ' ' && buf[idx] != '\0') 
        idx ++;

    // parse kernel address
    while (buf[idx] == ' ') idx++; // skip space
    __parse_number(buf, &idx, kernel, 1);

    // no-support rootfs address
    while (buf[idx] == ' ') idx++;
    __parse_number(buf, &idx, 0, 1);

    // parse fdt address
    while (buf[idx] == ' ') idx++;
    __parse_number(buf, &idx, fdt, 1);

    // check if all required fields were parsed
    if (*kernel == 0 || *fdt == 0)
        return -EFAULT;
    return 0;
}

/**
 * memory display and write
 */
static void md(int *address, int length)
{
    int i;

    for (i = 0; i < length; i++) {
        printf("%08x ", address[i]);
        if (i % 4 == 3)
            printf("\r\n");
    }
    printf("\r\n");
}

static void mw(int *address, int value)
{
    *address = value;
}

static int parse_mm(const char *buf, int *address, int *length, int hex)
{
    int idx = 0;

    while (buf[idx] != ' ' && buf[idx] != '\0') 
        idx ++;

    // parse address
    while (buf[idx] == ' ') idx++;
    __parse_number(buf, &idx, address, 1);

    // parse value
    while (buf[idx] == ' ') idx++;
    __parse_number(buf, &idx, length, hex);

    if (length == 0)
        return -EFAULT;
    return 0;
}

/* cache control */
static int parse_cache(const char *buf)
{
    int idx = 0;
    char c;
    char status;

    while (buf[idx] != ' ' && buf[idx] != '\0') 
        idx ++;

    // parse i / d
    while (buf[idx] == ' ') idx++;
    c = buf[idx++];

    // parse status: 0=(ASCII)48  i=(ASCII)105
    while (buf[idx] == ' ') idx++;
    status = buf[idx];

    if (c == 'i') 
    {
        if (status == 105) {
            SCB_InvalidateICache();
            printf("cache: icache invalid\r\n");
        } else if (status == 48) {
            SCB_DisableICache();
            printf("cache: icache disabled\r\n");
        } else {
            SCB_EnableICache();
            printf("cache: icache enabled\r\n");
        }
        return 0;
    }
    else if (c == 'd')
    {
        SCB_CleanDCache();
        if (status == 105) {
            SCB_InvalidateDCache();
            printf("cache: dcache invalid\r\n");
        }else if (status == 48) {
            SCB_DisableDCache();
            printf("cache: dcache disabled\r\n");
        } else {
            SCB_EnableDCache();
            printf("cache: dcache enabled\r\n");
        }
        return 0;
    }

    pr_info("error: no command arg %c", c);
    return -EFAULT;
}

/*
 * boot
 */
int do_boot(const char *buf)
{
    int kernel, fdt;

    if(parse_address(buf, &kernel, &fdt))
        return -EFAULT;
    kernel_entry(kernel, fdt);
    return 0;
}

void help_boot(void)
{
    println(L2, "boot <kernel address> - <fdt address>");
    println(L2, "use default address: boot ");
}
SHELL_EXPORT_CMD(boot, help_boot, do_boot);

/*
 * help
 */
int do_help(const char *buf)
{
    struct cmd *iter;
    printf("support command:\r\n");
    for (iter = head->next; iter; iter = iter->next) {
        printf("  %s\r\n", iter->name);
        if (iter->help)
            iter->help();
    }
    return 0;
}
SHELL_EXPORT_CMD(help, NULL, do_help);

/*
 * clear
 */
int do_clear(const char *buf)
{
    printf("\033[2J\033[H");
    return 0;
}
SHELL_EXPORT_CMD(clear, NULL, do_clear);

/*
 * md: memory display
 */
int do_md(const char *buf)
{
    int address, length;

    if (parse_mm(buf, &address, &length, 0))
        return -EFAULT;
    md((int *)address, length);
    return 0;
}

void help_md(void)
{
    println(L2, "md <address> <length>");
    println(L2, "read CPUID: md e000ed00 1");
    println(L2, "411fc271");
}
SHELL_EXPORT_CMD(md, help_md, do_md);

/*
 * mw: memory write
 */
int do_mw(const char *buf)
{
    int address, value;

    if (parse_mm(buf, &address, &value, 1))
        return -EFAULT;
    mw((int *)address, value);
    return 0;
}

void help_mw(void)
{
    println(L2, "mw <address> <value>");
}
SHELL_EXPORT_CMD(mw, help_mw, do_mw);

/*
 * mtest
 */
int do_mtest(const char *buf)
{
    memory_speed_test();
    return 0;
}
SHELL_EXPORT_CMD(mtest, NULL, do_mtest);

/*
 * cache
 */
int do_cache(const char *buf)
{
    parse_cache(buf);
    return 0;
}

void help_cache(void)
{
    println(L2, "cache <name> <op>");
    println(L2, "disable icache: cache i 0");
    println(L2, "invalid dcache: cache d i");
}
SHELL_EXPORT_CMD(cache, help_cache, do_cache);

/*
 * version
 */
int do_version(const char *buf)
{
    printf("st-boot %s\r\n", STBOOT_VERSION);
    return 0;
}
SHELL_EXPORT_CMD(version, NULL, do_version);

/*
 * reset
 */
int do_reset(const char *buf)
{
    NVIC_SystemReset();
    return 0;
}
SHELL_EXPORT_CMD(reset, NULL, do_reset);

/*
 * set qspi-flash memory mapped mode
 */
void qftool_map(void)
{
    if (QSPI_W25Qxx_MMMode()) {
        pr_info("Error: failed to map qspi-flash to memory space");
    }
    pr_info("qspi: memory mapped success");
}

void qftool_unmap(void)
{
    extern QSPI_HandleTypeDef hqspi;

    HAL_QSPI_Abort(&hqspi);
    QSPI_W25Qxx_Init();
    pr_info("qspi: memory unmapped");
}

int do_qftool(const char *buf)
{
    int idx = 0;
    const char *arg;

    while (buf[idx] != ' ' && buf[idx] != '\0') 
        idx ++;

    // parse fdt / kernel
    while (buf[idx] == ' ') idx++;
    arg = &buf[idx];

    switch (arg[0]) {
    case 'm':
        qftool_map();
        break;
    case 'u':
        qftool_unmap();
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

void help_qftool(void)
{
    println(L2, "qftool <map/unmap>");
    println(L2, "a tool for controlling qspi-flash");
}
SHELL_EXPORT_CMD(qftool, help_qftool, do_qftool);

/*
 * update fdt / kernel
 *
 * slice the end 4KB sector (0xf000) from 64KB Block storing dtb,
 * for recording current erasing and writing status of the image
 * a bitmap shows below:
 *      0000 0000 0000 0000 0011 1111 1111 1111
 *      1111 1111 1111 1111 1111 1111 1111 1111
 *      1111 1111 1111 1111 1111 1111 1111 1111
 *      1111 1111 1111 1111 1111 1111 1111 1111
 * total 128 bits for record 128 blocks of w25q64, the figure above represents
 *  block 0 - 17 had been writen succeed,
 * if reupdate, image will be write starting from block 18th
 *
 * update status:
 * [default]    0xff
 * [writing]    0xaa
 * [finish]     0x00
 *  status byte will be erase after succeed (implicit erase, actual called before next full write)
 *
 */
int update_fdt(void)
{
    unsigned char *fdt;
    int size;
    int ret;

    // read from sd
    ret = sdmmc_read_file("0:fdt", &fdt, &size);

    if (ret == FR_OK) {
        // write into qspi-flash
        printf("image size: %3.2fKB, erasing flash ...\r\n", (float)size/1024);
        QSPI_W25Qxx_BlockErase_64K(FDT_ADDR-QSPI_FLASH_BASE_ADDR);
        printf("writing dtb ...\r\n");
        ret = QSPI_W25Qxx_WriteBuffer(fdt, FDT_ADDR-QSPI_FLASH_BASE_ADDR, size);
        if (ret) {
            printf("Error: %d in writing qspi-flash\r\n", ret);
            return -EIO;
        }
        printf("update fdt success\r\n");
    }
    return 0;
}

int update_kernel(void)
{
#define BITMAP_SECTOR  0xf000
#define BITMAP_SIZE    16
#define BITMAP_END     0xffff
    unsigned char *image_buffer;
    int size = 0;
    int ret;
    int eaddr = KERNEL_ADDR-QSPI_FLASH_BASE_ADDR;
    int i;
    uint8_t block_bitmap[BITMAP_SIZE];
    uint8_t calib;

    QSPI_W25Qxx_ReadBuffer(&calib, BITMAP_SECTOR+BITMAP_SIZE, 1);

    if (calib == 0xaa) { // need recover
        // read bitmap
        ret = QSPI_W25Qxx_ReadBuffer(block_bitmap, BITMAP_SECTOR, sizeof(block_bitmap));
        if (ret) {
            printf("Error: %d in reading bitmap\r\n", ret);
            return -EIO;
        }
        // search re-startup block
        for (i = 127; i >= 0; i--) {
            if (block_bitmap[i / 8] >> (7 - i % 8) == 0)
                break;
        }
        eaddr = (i + 1) * 0x10000;
    } else {
        // erase bitmap sector, change calib
        QSPI_W25Qxx_SectorErase(BITMAP_SECTOR);
        memset(block_bitmap, 0xff, sizeof(block_bitmap));
        QSPI_W25Qxx_WritePage(&(uint8_t){0xaa}, BITMAP_SECTOR+BITMAP_SIZE, 1);
    }

    // read from sd
    ret = sdmmc_read_file("0:kernel", &image_buffer, &size);

    if (ret == FR_OK) {
        printf("image size: %3.2fMB, ready to erase flash:\r\n", (float)size/1024/1024);

        while (eaddr < size) {
            i = (eaddr >> 16) - 1;

            // erase blocks
            printf("\rerasing flash block  [%3d]", i);
            QSPI_W25Qxx_BlockErase_64K(eaddr);

            // write into qspi-flash
            printf("\rwriting kernel image [%3d]", i);
            ret = QSPI_W25Qxx_WriteBuffer(image_buffer + i * 0x10000, eaddr, 0x10000);
            if (ret) {
                printf("\r\nError: %d in writing qspi-flash\r\n", ret);
                return -EIO;
            }

            // bitmap
            block_bitmap[i / 8] &= ~(1 << (7 - i % 8));
            QSPI_W25Qxx_WritePage(block_bitmap, BITMAP_SECTOR, sizeof(block_bitmap));

            eaddr += 0x10000;
        }

        ret = QSPI_W25Qxx_WritePage(&(uint8_t){0x00}, BITMAP_SECTOR+BITMAP_SIZE, 1);
        if (ret) {
            printf("Error: %d in writing calibration\r\n", ret);
            return -EIO;
        }
        printf("\r\nupdate kernel success\r\n");
    }
    return 0;
}

int do_update(const char *buf)
{
    int idx = 0;
    const char *arg;

    while (buf[idx] != ' ' && buf[idx] != '\0')
        idx ++;

    // parse fdt / kernel
    while (buf[idx] == ' ') idx++;
    arg = &buf[idx];
    
    switch (arg[0]) {
    case 'f':
        update_fdt();
        break;
    case 'k':
        update_kernel();
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

void help_update(void)
{
    println(L2, "update <fdt/kernel>");
    println(L2, "! need you modify the image file name to \"fdt\" or \"kernel\" in advance");
}
SHELL_EXPORT_CMD(update, help_update, do_update);