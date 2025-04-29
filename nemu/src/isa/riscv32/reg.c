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

#include "local-include/reg.h"

#include <isa.h>
#include <stdlib.h>
#include <string.h>

const char* regs[] = {"$0", "ra", "sp",  "gp",  "tp", "t0", "t1", "t2",
                      "s0", "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
                      "a6", "a7", "s2",  "s3",  "s4", "s5", "s6", "s7",
                      "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

void isa_reg_display() {
    printf("Register Display:\n");
    printf("==========================================\n");

    // Print PC register
    printf("pc: 0x%08x\n", cpu.pc);

    // Print general purpose registers
    for (int i = 0; i < 32; i++) {
        printf("%-3s: 0x%08x  ", reg_name(i), gpr(i));

        // Format output: 4 registers per line
        if ((i + 1) % 4 == 0) {
            printf("\n");
        }
    }

    // Add a newline if we didn't end on one
    if (32 % 4 != 0) {
        printf("\n");
    }

    printf("==========================================\n");
}

uint32_t isa_reg_str2val(const char* s, bool* success) {
    *success = true;

    // Handle PC register separately
    if (strcmp(s, "pc") == 0) {
        return cpu.pc;
    }

    // Check for numeric register format: $number
    if (s[0] == '$') {
        char* endptr;
        long idx =
            strtol(s + 1, &endptr, 10);  // Skip the '$' and convert to number

        // Check if conversion was successful and within range
        if (*endptr == '\0' && idx >= 0 && idx < 32) {
            return gpr(idx);
        }
    }

    // Check for register name in the regs array
    for (int i = 0; i < 32; i++) {
        // Handle special case for "$0" (zero register)
        if (i == 0 && strcmp(s, "zero") == 0) {
            return 0;  // x0 is hardwired to 0
        }

        // For other registers, compare with name in the regs array (skip the $
        // for index 0)
        if (strcmp(s, regs[i] + (i == 0 ? 1 : 0)) == 0) {
            return gpr(i);
        }
    }

    // If we get here, the register name was not recognized
    *success = false;
    return 0;
}
