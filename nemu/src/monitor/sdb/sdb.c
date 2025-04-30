/***************************************************************************************
 * Copyright (c) 2014-2024 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 *PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 *KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 *NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include "sdb.h"

#include <cpu/cpu.h>
#include <isa.h>
#include <memory/vaddr.h>
#include <readline/history.h>
#include <readline/readline.h>

static int is_batch_mode = true;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin.
 */
static char* rl_gets() {
    static char* line_read = NULL;

    if (line_read) {
        free(line_read);
        line_read = NULL;
    }

    line_read = readline("(nemu) ");

    if (line_read && *line_read) {
        add_history(line_read);
    }

    return line_read;
}

static int cmd_c(char* args) {
    cpu_exec(-1);
    return 0;
}

static int cmd_q(char* args) {
    return -1;
}

static int cmd_help(char* args);
static int cmd_si(char* args);
static int cmd_info(char* args);
static int cmd_x(char* args);
static int cmd_p(char* args);
static int cmd_w(char* args);
static int cmd_d(char* args);

static struct {
    const char* name;
    const char* description;
    int (*handler)(char*);
} cmd_table[] = {
    {"help", "Display information about all supported commands", cmd_help},
    {"c", "Continue the execution of the program", cmd_c},
    {"q", "Exit NEMU", cmd_q},
    {"si", "Step N instructions then pause, default N=1", cmd_si},
    {"info", "Print program state: r(registers), w(watchpoints)", cmd_info},
    {"x",
     "Examine memory: x N EXPR (N consecutive 4-byte units from EXPR address)",
     cmd_x},
    {"p", "Print expression value: p EXPR (e.g., p $eax + 1)", cmd_p},
    {"w", "Set watchpoint: w EXPR (stop when EXPR changes)", cmd_w},
    {"d", "Delete watchpoint: d N (delete watchpoint number N)", cmd_d},
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char* args) {
    /* extract the first argument */
    char* arg = strtok(NULL, " ");
    int i;

    if (arg == NULL) {
        /* no argument given */
        for (i = 0; i < NR_CMD; i++) {
            printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        }
    } else {
        for (i = 0; i < NR_CMD; i++) {
            if (strcmp(arg, cmd_table[i].name) == 0) {
                printf("%s - %s\n", cmd_table[i].name,
                       cmd_table[i].description);
                return 0;
            }
        }
        printf("Unknown command '%s'\n", arg);
    }
    return 0;
}

// Implement the single-step command
static int cmd_si(char* args) {
    /* Execute N instructions */
    int n = 1;

    if (args != NULL) {
        char* nptr;
        int val = strtol(args, &nptr, 10);
        if (nptr != args && val > 0) {
            n = val;
        }
    }

    printf("Executing %d instruction(s)...\n", n);
    cpu_exec(n);
    return 0;
}

// Implement the info command
static int cmd_info(char* args) {
    if (args == NULL) {
        printf(
            "'info' requires an argument (r for registers, w for "
            "watchpoints)\n");
        return 0;
    }

    // Skip leading spaces
    while (*args && isspace(*args)) {
        args++;
    }

    if (*args == 'r') {
        // Display register values
        isa_reg_display();
    } else if (*args == 'w') {
        // Display watchpoint information
        print_watchpoints();
    } else {
        printf("Unknown info command '%s'\n", args);
    }

    return 0;
}

// Implement the memory examine command
static int cmd_x(char* args) {
    if (args == NULL) {
        printf("'x' requires arguments: x N EXPR\n");
        return 0;
    }

    // Parse the number of units to examine
    char* n_str = strtok(args, " ");
    if (n_str == NULL) {
        printf("Missing arguments. Usage: x N EXPR\n");
        return 0;
    }

    int n = strtol(n_str, NULL, 10);
    if (n <= 0) {
        printf("Invalid number of units: %s\n", n_str);
        return 0;
    }

    // Get the expression string (everything after the first space)
    char* expr_str = n_str + strlen(n_str) + 1;
    while (*expr_str && isspace(*expr_str)) {
        expr_str++;  // Skip leading spaces
    }

    if (*expr_str == '\0') {
        printf("Missing expression. Usage: x N EXPR\n");
        return 0;
    }

    // Evaluate the expression to get the memory address
    bool success = true;
    vaddr_t addr = expr(expr_str, &success);

    if (!success) {
        printf("Failed to evaluate expression: %s\n", expr_str);
        return 0;
    }

    // Print memory contents
    printf("Memory at 0x%08x:\n", addr);
    for (int i = 0; i < n; i++) {
        // Get 4 bytes from memory
        uint32_t data = vaddr_read(addr + i * 4, 4);
        printf("0x%08x: 0x%08x\n", addr + i * 4, data);
    }

    return 0;
}

// Implement the expression evaluation command
static int cmd_p(char* args) {
    if (args == NULL) {
        printf("'p' requires an expression argument\n");
        return 0;
    }

    bool success = true;
    uint32_t result = expr(args, &success);

    if (success) {
        printf("Expression value: 0x%08x (%u)\n", result, result);
    } else {
        printf("Failed to evaluate expression: %s\n", args);
    }

    return 0;
}

// Implement the watchpoint command
static int cmd_w(char* args) {
    if (args == NULL) {
        printf("'w' requires an expression argument\n");
        return 0;
    }

    // Set the watchpoint
    set_watchpoint(args);
    return 0;
}

// Implement the delete watchpoint command
static int cmd_d(char* args) {
    if (args == NULL) {
        printf("'d' requires a watchpoint number\n");
        return 0;
    }

    char* endptr;
    int num = strtol(args, &endptr, 10);

    if (*endptr != '\0' || endptr == args) {
        printf("Invalid watchpoint number: %s\n", args);
        return 0;
    }

    delete_watchpoint(num);
    return 0;
}

void sdb_set_batch_mode() {
    is_batch_mode = true;
}

void sdb_mainloop() {
    if (is_batch_mode) {
        cmd_c(NULL);
        return;
    }

    for (char* str; (str = rl_gets()) != NULL;) {
        char* str_end = str + strlen(str);

        /* extract the first token as the command */
        char* cmd = strtok(str, " ");
        if (cmd == NULL) {
            continue;
        }

        /* treat the remaining string as the arguments,
         * which may need further parsing
         */
        char* args = cmd + strlen(cmd) + 1;
        if (args >= str_end) {
            args = NULL;
        }

#ifdef CONFIG_DEVICE
        extern void sdl_clear_event_queue();
        sdl_clear_event_queue();
#endif

        int i;
        for (i = 0; i < NR_CMD; i++) {
            if (strcmp(cmd, cmd_table[i].name) == 0) {
                if (cmd_table[i].handler(args) < 0) {
                    return;
                }
                break;
            }
        }

        if (i == NR_CMD) {
            printf("Unknown command '%s'\n", cmd);
        }
    }
}

void init_sdb() {
    /* Compile the regular expressions. */
    init_regex();

    /* Initialize the watchpoint pool. */
    init_wp_pool();
}