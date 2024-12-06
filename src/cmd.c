/**
 * @file cmd.rc
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

#define N_CMD          4
#define CMD_SPACE_L1   "    "
#define CMD_SPACE_L2   "        "

// echo: 回显
#define CMD_ECHO(rc) \
    printf("%s%s \b", &rc, &buf[i_arrow]);\
    for (b = 0; b < i - i_arrow; b++)\
        putchar('\b')

static int parse_address(const char *, char *, int *, int *);
static char command[N_CMD][8] = {
    "boot",
    "help",
    "clear",
    "rdreg", //TODO: read peripheral register
};

/**
 ** read command to buffer
 *
 * DEL: Delete key: \033[ ... ~
 */
void command_read(char *buf)
{
    int i = 0, b;
    int i_arrow = i;
    char rc;

    while (1) {
    rc = getchar();

    switch (rc) {
    case '\033': // Arrow and DEL
        getchar();
        switch (getchar()) {
        case 'B':
        case 'C': // right
            if (i_arrow < i) {
                printf("\033[C");
                i_arrow++;
            }
            break;
        case 'A':
        case 'D': // left
            if (i_arrow > 0) {
                printf("\033[D");
                i_arrow--;
            }
            break;
        default:
            while ((rc = getchar()) != '~');
        }
        continue;
    case 127: // Backspace
        if (i > 0) {
            CMD_ECHO(*"\b \b");
            memmove(&buf[i_arrow-1], &buf[i_arrow], i-i_arrow);
            buf[--i] = '\0';
            i_arrow--;
        }
        if (!i) buf[i] = '\0';
        continue;
    case '\r': // Enter
        printf("\r\n");
        return;
    default:
        CMD_ECHO(rc);
        // insert
        if (i == i_arrow) {
            buf[i++] = rc;
            i_arrow++;
        } else {
            memmove(&buf[i_arrow+1], &buf[i_arrow], i-i_arrow);
            buf[i_arrow++] = rc;
            i++;
        }
    }
    }
}

/*
 * parse command input
 */
int parse_command(const char *buf, char *cmd, int *kernel, int *fdt) {
    int i;

    if (!strncmp(buf, command[0], strlen(command[0]))) {
        // boot
        if(parse_address(buf, cmd, kernel, fdt))
            return -EFAULT;
        return 0;
    } else if (!strncmp(buf, command[1], strlen(command[1]))) {
        // help
        printf("support command:\r\n");
        for (i = 0; i < N_CMD; i++) {
            printf(CMD_SPACE_L1"%s\r\n", command[i]);
            if (!strncmp(command[i], "boot", strlen("boot")))
                printf(CMD_SPACE_L2"boot <kernel address> - <fdt address>\r\n");
        }
    } else if (!strncmp(buf, command[2], strlen(command[2])))
        // clear
        printf("\033[2J\033[H");
    else
        // unknown
        pr_info("error: %s: no such a command", buf);
    console_cmd();
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
    int i = 0;

    // parse command
    while (buf[i] != ' ' && buf[i] != '\0') {
        cmd[i] = buf[i];
        i++;
    }
    cmd[i] = '\0';

    // parse kernel address
    while (buf[i] == ' ') i++; // skip space
    __parse_address(buf, &i, kernel);

    // no-support rootfs address
    while (buf[i] == ' ') i++;
    __parse_address(buf, &i, 0);

    // parse fdt address
    while (buf[i] == ' ') i++;
    __parse_address(buf, &i, fdt);

    // check if all required fields were parsed
    if (cmd[0] == '\0' || *kernel == 0 || *fdt == 0)
        return -EFAULT;
    return 0;
}