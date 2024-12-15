#ifndef SHELL_COMMAND_H
#define SHELL_COMMAND_H


struct cmd {
    char *name;
    void (*help)(void);
    int (*exec)(const char *buf);
    struct cmd *next;
};

// export command
#define SHELL_EXPORT_CMD(cmd_name, help_func, exec_func) \
    __attribute__((used, section(".shell_cmd"))) \
    static struct cmd __cmd_##cmd_name = { \
        .name = #cmd_name, \
        .help = help_func, \
        .exec = exec_func \
    }

// command help println
#define CMD_SPACE_L2      "        "
#define println(level, fmt) printf(CMD_SPACE_##level"%s\r\n",fmt)


#endif