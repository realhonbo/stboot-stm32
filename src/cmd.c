/**
 * @file cmd.c
 * @author Honbo (hehongbo918@gmail.com)
 * @brief process command for console input
 * @version 1.0
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <stm32h7xx_hal.h>
#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "errno.h"

#define N_CMD               8
#define MAX_HISTORY_LINE   32
#define CMD_SPACE_L1      "  "
#define CMD_SPACE_L2      "        "

static char command[N_CMD][8] = {
        "boot",
        "help",
        "clear",
        "md",    /* mem display */
        "mw",    /* mem write */
        "mtest", /* mem test */
        "cache", /* cache control */
        "version",
};

/** cmd cache */
struct cc_cache {
    char *cache[MAX_HISTORY_LINE][64];
    int ptr;
    int flag;
};

static struct cc_cache ccache;

// command help println
#define println(level, fmt) printf(CMD_SPACE_##level"%s\r\n",fmt)

// compare prefix of buf with commands
#define CMD_CMP(i)\
    strncmp(buf, command[i], strlen(command[i]))

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

// store command history to cmd_cache
static void storeto_cmd_cache(const char *buf)
{
    if (!(ccache.ptr < MAX_HISTORY_LINE))
        ccache.ptr = -1;
    ccache.ptr ++;
    memcpy(ccache.cache[ccache.ptr], buf, 64);
}

static void load_lastcmd(char *buf)
{
    int i;

    if (ccache.ptr - ccache.flag < 0 ||
            !ccache.cache[ccache.ptr-ccache.flag][0])
        return;
    memcpy(buf, ccache.cache[ccache.ptr-ccache.flag], 64);
    ccache.flag ++;
}

static void command_read(char *buf);
static int parse_command(const char *);
static int parse_address(const char *, char *, int *, int *);
static int parse_mm(const char *, char *, int *, int *);
static void md(int *, int );
static void mw(int *, int);
static int parse_cache(const char *, char *);

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
    char buf[64];

#ifdef CONSOLE_CMD
input:
    memset(buf, 0, 64);
    printf("st-boot > ");
    setvbuf(stdin,  NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    command_read(buf);

    // solve commands
    if (parse_command(buf))
        return -EFAULT;
    goto input;
#endif

    kernel_entry(0, 0);
    return 0;
}

/**
 ** read command to buffer
 *
 * DEL: Delete key: \033[ ... ~
 *
 * IDX: idx_arrow should be after the character you will change
 *      "ABC<idx_arrow>DEFGH<idx>"
 */
static void command_read(char *buf)
{
    int idx = 0;
    int idx_arrow = idx;
    int i, len;
    char rc;

    while (1) {
    rc = getchar();

    switch (rc) {
    case '\033': // Arrow and DEL
        getchar();
        switch (getchar()) {
        case 'B':
        case 'C': // right
            if (idx_arrow < idx) {
                printf("\033[C");
                idx_arrow++;
            }
            break;
        case 'A': // last command
            for(i = 0; i < strlen(buf); i++)
                printf("\b \b");
            load_lastcmd(buf);
            idx = strlen(buf);
            idx_arrow = idx;
            printf("%s", buf);
            break;
        case 'D': // left
            if (idx_arrow > 0) {
                printf("\033[D");
                idx_arrow--;
            }
            break;
        default:
            while ((rc = getchar()) != '~');
        }
        continue;
    case 127: // Backspace
        if (idx > 0) {
            printf("\b \b%s \b", &buf[idx_arrow]);
            for (i = 0; i < idx - idx_arrow; i++)
                putchar('\b');
            memmove(&buf[idx_arrow-1], &buf[idx_arrow], idx-idx_arrow);
            buf[--idx] = '\0';
            --idx_arrow;
        }
        if (!idx) buf[idx] = '\0';
        continue;
    case '\t': // Tab Complete
        for (i = 0; i < N_CMD; i++) {
            if (!strncmp(buf, command[i], idx)) {
                len = strlen(command[i]+idx);
                printf("%s", command[i]+idx);
                memcpy(&buf[idx], command[i]+idx, len);
                idx += len;
                idx_arrow += len;
            }
        }
        continue;
    case '\r': // Enter
        printf("\r\n");
        storeto_cmd_cache(buf);
        ccache.flag = 0;
        return;
    case 3: // ctrl-c
        printf("\r\n");
        memset(buf, 0, strlen(buf));
        return;
    default:
        printf("%c%s \b", rc, &buf[idx_arrow]);
        for (i = 0; i < idx - idx_arrow; i++)
            putchar('\b');
        // insert
        if (idx == idx_arrow) {
            buf[idx++] = rc;
            idx_arrow++;
        } else {
            memmove(&buf[idx_arrow+1], &buf[idx_arrow], idx-idx_arrow);
            buf[idx_arrow++] = rc;
            idx++;
        }
    }
    }
}

/*
 * parse command input
 */
static int parse_command(const char *buf) {
    int i;
    char cmd[8] = "";
    int kernel, fdt;
    int address;
    int length, value;

    if (!strlen(buf))
        goto out;

    if (!CMD_CMP(0))
    {   // boot
        if(parse_address(buf, cmd, &kernel, &fdt))
            return -EFAULT;
        kernel_entry(kernel, fdt);
        return 0;
    }
    else if (!CMD_CMP(1))
    {   // help
        printf("support command:\r\n");
        for (i = 0; i < N_CMD; i++) {
            println(L1, command[i]);
            if (!strncmp(command[i], "boot", 4)) {
                println(L2, "boot <kernel address> - <fdt address>");
                println(L2, "use default address: boot ");
            }
            else if (!strncmp(command[i], "md", 2)) {
                println(L2, "md <address> <length>");
                println(L2, "read CPUID: md e000ed00 1");
                println(L2, "411fc271");
                println(L2, "! Both address and length are hex format");
            }
            else if (!strncmp(command[i], "mw", 2)) {
                println(L2, "mw <address> <value>");
            }
            else if (!strncmp(command[i], "cache", 5)) {
                println(L2, "cache <name> <op>");
                println(L2, "disable icache: cache i 0");
                println(L2, "invalid dcache: cache d i");
            }
        }
        printf("\r\n");
    }
    else if (!CMD_CMP(2))
        // clear
        printf("\033[2J\033[H");
    else if (!CMD_CMP(3))
    {   // md
        if (parse_mm(buf, cmd, &address, &length))
            return -EFAULT;
        md((int *)address, length);
    }
    else if (!CMD_CMP(4))
    {   // mw
        if (parse_mm(buf, cmd, &address, &value))
            return -EFAULT;
        mw((int *)address, value);
    }
    else if (!CMD_CMP(5)) 
        // mtest
        memory_speed_test();
    else if (!CMD_CMP(6))
        // cache
        parse_cache(buf, cmd);
    else if (!CMD_CMP(7))
        // version
        printf("st-boot %s\r\n", STBOOT_VERSION);
    else
        // unknown
        pr_info("error: %s: no such a command", buf);
out:
    return 0;
}


/*
 * parse address of `boot`
 */
static void __parse_address(const char *buf, int *idx, int *address)
{
    int i = *idx;
    *address = 0;

    if (buf[i] == '-') i++;

    if (!strncmp(&buf[i], "0x", 2)) i += 2;
    while (buf[i] != ' ' && buf[i] != '\0') {
        if (buf[i] >= '0' && buf[i] <= '9')
            *address = (*address * 16) + (buf[i] - '0');
        else if (buf[i] >= 'a' && buf[i] <= 'f')
            *address = (*address * 16) + (buf[i] - 'a' + 10);
        else if (buf[i] >= 'A' && buf[i] <= 'F')
            *address = (*address * 16) + (buf[i] - 'A' + 10);
        else
            break; // non-hex character, break out of the loop
        i++;
    }
    *idx = i;
}

/**
 * instead of: sscanf(buf, "%s %x - %x", cmd, &kernel, &fdt) != 3
 */
static int parse_address(const char *buf, char *cmd, int *kernel, int *fdt) {
    int idx = 0;
    *kernel = 0;
    *fdt = 0;

    if (strlen(buf) < 9)
        return 0;

    // parse command
    parse_cmd_name(buf, cmd, &idx);

    // parse kernel address
    while (buf[idx] == ' ') idx++; // skip space
    __parse_address(buf, &idx, kernel);

    // no-support rootfs address
    while (buf[idx] == ' ') idx++;
    __parse_address(buf, &idx, 0);

    // parse fdt address
    while (buf[idx] == ' ') idx++;
    __parse_address(buf, &idx, fdt);

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

static int parse_mm(const char *buf, char *cmd, int *address, int *length)
{
    int idx = 0;

    // jump over name
    parse_cmd_name(buf, cmd, &idx);

    // parse address
    while (buf[idx] == ' ') idx++;
    __parse_address(buf, &idx, address);

    // parse length / value
    while (buf[idx] == ' ') idx++;
    __parse_address(buf, &idx, length);

    if (length == 0)
        return -EFAULT;
    return 0;
}

/* cache control */
static int parse_cache(const char *buf, char *cmd)
{
    int idx = 0;
    char c;
    char status;

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