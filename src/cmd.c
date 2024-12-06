/**
 * @file cmd.c
 * @author Honbo (hehongbo918@gmail.com)
 * @brief process command for console input
 * @version 1.0
 * 
 * @copyright Copyright (rc) 2024
 * 
 */
#include <stdio.h>
#include <string.h>
#include "bsp.h"
#include "errno.h"

#define N_CMD               6
#define MAX_HISTORY_LINE   32
#define CMD_SPACE_L1   "    "
#define CMD_SPACE_L2   "        "

static char command[N_CMD][8] = {
        "boot",
        "help",
        "clear",
        "md",
        "mtest",
        "info",
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
static int parse_command(const char *, char *);
static int parse_address(const char *, char *, int *, int *);
static int parse_md(const char *, char *, int *, int *);
static void md(int *, int );

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
    char buf[64] = "", cmd[8] = "";

#ifdef CONSOLE_CMD
    printf("st-boot > ");
    setvbuf(stdin,  NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    command_read(buf);

    // solve commands
    if (parse_command(buf, cmd))
        return -EFAULT;
#endif
    return 0;
}

/**
 ** read command to buffer
 *
 * DEL: Delete key: \033[ ... ~
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
            idx_arrow--;
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
static int parse_command(const char *buf, char *cmd) {
    int i;
    int kernel = 0, fdt = 0;
    int address, length;

    if (!strlen(buf) || !CMD_CMP(0))
    { // boot
        if(parse_address(buf, cmd, &kernel, &fdt))
            return -EFAULT;
        kernel_entry(kernel, fdt);
        return 0;
    }
    else if (!CMD_CMP(1))
    { // help
        printf("support command:\r\n");
        for (i = 0; i < N_CMD; i++) {
            println(L1, command[i]);
            if (!strncmp(command[i], "boot", 4))
                println(L2, "boot <kernel address> - <fdt address>");
            else if (!strncmp(command[i], "md", 2)) {
                println(L2, "md <address>  <length>");
                println(L2, "read CPUID: md e000ed00 1");
                println(L2, "411fc271");
                println(L2, "! Both address and length are hex format");
            }
        }
        printf("\r\n");
    }
    else if (!CMD_CMP(2))
    { // clear
        printf("\033[2J\033[H");
    }
    else if (!CMD_CMP(3))
    { // md
        if (parse_md(buf, cmd, &address, &length))
            return -EFAULT;
        md((int *)address, length);
    }
    else if (!CMD_CMP(4))
    {
        memory_speed_test();
    }
    else
    { // unknown
        pr_info("error: %s: no such a command", buf);
    }
    console_cmd();
    return 0;
}


/*
 * parse address of `boot`
 */
static void __parse_address(const char *buf, int *idx, int *address)
{
    int i = *idx;

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
 * memory display
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

static int parse_md(const char *buf, char *cmd, int *address, int *length)
{
    int idx = 0;

    // jump over name
    parse_cmd_name(buf, cmd, &idx);

    // parse address
    while (buf[idx] == ' ') idx++;
    __parse_address(buf, &idx, address);

    // parse length
    while (buf[idx] == ' ') idx++;
    __parse_address(buf, &idx, length);

    if (length == 0)
        return -EFAULT;
    return 0;
}