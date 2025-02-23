/**
 * @file shell.c
 * @author Honbo (hehongbo918@gmail.com)
 * @brief process command for shell input
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

/*
 * ascii escape character code
 */
#define hide_cursor()   printf("\033[?25l")
#define show_cursor()   printf("\033[?25h");

static void backspace(int ch)
{
    // default 1 -> although if 0: backspace 1
    if (ch > 0) {
        printf("\033[%dD", ch);
    }
}

static void move_cursor(int ch)
{
    printf("\033[%dG", ch);
}

static void clear_to_eol(void)
{
    printf("\033[K");
}


/*
 * shell
 */
#define MAX_HISTORY_LINE   32
#define MAX_CMD_LENGTH     64

struct cmd *head;

static void add_command(struct cmd *command)
{
    command->next = head->next;
    head->next = command;
}

// cmd cache
struct cc_cache {
    char cache[MAX_HISTORY_LINE][MAX_CMD_LENGTH];
    int tail; // after last elem
    int curr; // elem be read
};

static struct cc_cache ccache;

// store command history to cmd_cache
static void storeto_cmd_cache(const char *buf)
{
    memcpy(ccache.cache[ccache.tail], buf, MAX_CMD_LENGTH);
    ccache.tail = (ccache.tail + 1) % MAX_HISTORY_LINE;
    ccache.curr = ccache.tail;
}

static inline void load_lastcmd(char *buf)
{
    ccache.curr --;
    if (ccache.curr < 0) {
        ccache.curr = 0;
        return;
    }
    memcpy(buf, ccache.cache[ccache.curr], MAX_CMD_LENGTH);
}

static inline void load_nextcmd(char *buf)
{
    ccache.curr ++;
    if (ccache.curr > ccache.tail) {
        ccache.curr = ccache.tail;
        return;
    }
    memcpy(buf, ccache.cache[ccache.curr], MAX_CMD_LENGTH);
}

static void command_read(char *buf);
static void parse_command(const char *);

/**
 ** init commands from section `.shell_cmd`
 * append all to cmd linklist
 */
void commands_init(void)
{
    extern struct cmd _shell_cmd_start,
                      _shell_cmd_end;
    struct cmd *current = &_shell_cmd_start;

    head = malloc(sizeof(struct cmd));
    head->next = NULL;

    while (current < &_shell_cmd_end) {
        add_command(current);
        current ++;
    }
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
 ** the other functions will be inline to console_cmd
 */
__itcm void console_cmd(void)
{
    char buf[MAX_CMD_LENGTH];

#ifdef CONSOLE_CMD
    commands_init();
    setvbuf(stdin,  NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

input:
    memset(buf, 0, MAX_CMD_LENGTH);
    printf("=> ");
    command_read(buf);
    parse_command(buf);
    goto input;
#endif

    kernel_entry(0, 0);
}

/**
 ** read command to buffer
 *
 * DEL: Delete key: \033[ ... ~
 *
 * IDX: adx(arrow idx) should be after the character you will change
 *      "ABC<adx>DEFGH<idx>"
 */
static void command_read(char *buf)
{
    int idx = 0;
    int adx = idx;
    int i, len;
    struct cmd *iter;
    char rc;

    while (1) {
    rc = getchar();

    switch (rc) {
    case '\033': // Arrow and DEL
        getchar();
        switch (getchar()) {
        case 'B': // next command
            backspace(strlen(buf));
            clear_to_eol();
            load_nextcmd(buf);
            idx = strlen(buf);
            adx = idx;
            printf("%s", buf);
            break;
        case 'C': // right
            if (adx < idx) {
                printf("\033[C");
                adx++;
            }
            break;
        case 'A': // last command
            backspace(strlen(buf));
            clear_to_eol();
            load_lastcmd(buf);
            idx = strlen(buf);
            adx = idx;
            printf("%s", buf);
            break;
        case 'D': // left
            if (adx > 0) {
                backspace(1);
                adx--;
            }
            break;
        default:
            while ((rc = getchar()) != '~');
        }
        continue;
    case 127: // Backspace key
        if (adx > 0) {
            printf("\b \b%s \b", &buf[adx]);
            backspace(idx - adx);
            memmove(&buf[adx-1], &buf[adx], idx-adx);
            buf[--idx] = '\0';
            --adx;
        }
        if (!idx) buf[idx] = '\0';
        continue;
    case '\t': // Tab Complete
        for (iter = head->next; iter; iter = iter->next)
            if (!strncmp(buf, iter->name, idx)) {
                len = strlen(iter->name+idx);
                printf("%s", iter->name+idx);
                memcpy(&buf[idx], iter->name+idx, len);
                idx += len;
                adx += len;
            }
        continue;
    case '\r': // Enter
        printf("\r\n");
        if (strlen(buf)) {
            storeto_cmd_cache(buf);
            ccache.curr = ccache.tail;
        }
        return;
    case 3: // ctrl-c
        printf("\r\n");
        memset(buf, 0, strlen(buf));
        return;
    default:
        if (unlikely( idx >= MAX_CMD_LENGTH ))
            break;
        printf("%c%s \b", rc, &buf[adx]);
        backspace(idx - adx);
        // insert
        if (idx == adx) {
            buf[idx++] = rc;
            adx++;
        } else {
            memmove(&buf[adx+1], &buf[adx], idx-adx);
            buf[adx++] = rc;
            idx++;
        }
    }
    }
}

/*
 * parse command input
 */
static void parse_command(const char *buf) {
    struct cmd *iter;

    if (!strlen(buf))
        return;

    for (iter = head->next; iter; iter = iter->next)
        if (!strncmp(buf, iter->name, strlen(iter->name))) {
            iter->exec(buf);
            return;
        }

    printk(KERN_ERR "%s: no such a command", buf);
}