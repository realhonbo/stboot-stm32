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

extern struct cmd *head;

// get name from buf to cmd
static void parse_cmd_name(const char *buf, char *cmd, int *idx)
{
    int i = *idx;

    while (buf[i] != ' ' && buf[i] != '\0') {
        cmd[i] = buf[i];
        i++;
    }
    cmd[i] = '\0';
    *idx = i;
}

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
    char cmd[8] = "";
    int idx = 0;
    *kernel = 0;
    *fdt = 0;

    if (strlen(buf) < 9)
        return 0;

    // parse command
    parse_cmd_name(buf, cmd, &idx);

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
    if (cmd[0] == '\0' || *kernel == 0 || *fdt == 0)
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
    char cmd[8] = "";

    // jump over name
    parse_cmd_name(buf, cmd, &idx);

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
    char cmd[8] = "";

    parse_cmd_name(buf, cmd, &idx);

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