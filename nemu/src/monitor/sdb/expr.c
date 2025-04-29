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

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <memory/vaddr.h>
#include <regex.h>

enum {
    TK_NOTYPE = 256,
    TK_EQ,
    TK_NEQ,       // Not equal
    TK_AND,       // Logical AND
    TK_OR,        // Logical OR
    TK_NUM,       // Number
    TK_REG,       // Register
    TK_DEREF,     // Dereference operator
    TK_NEGATIVE,  // Negative sign (unary minus)
    TK_BIT_AND,   // Bitwise AND
    TK_BIT_OR,    // Bitwise OR
    TK_BIT_XOR,   // Bitwise XOR
};

static struct rule {
    const char* regex;
    int token_type;
} rules[] = {
    /* Pay attention to the precedence level of different rules */
    {" +", TK_NOTYPE},              // spaces
    {"\\+", '+'},                   // plus
    {"-", '-'},                     // minus
    {"\\*", '*'},                   // multiply or dereference
    {"/", '/'},                     // divide
    {"\\(", '('},                   // left parenthesis
    {"\\)", ')'},                   // right parenthesis
    {"==", TK_EQ},                  // equal
    {"!=", TK_NEQ},                 // not equal
    {"&&", TK_AND},                 // logical AND
    {"\\|\\|", TK_OR},              // logical OR
    {"&", TK_BIT_AND},              // bitwise AND
    {"\\|", TK_BIT_OR},             // bitwise OR
    {"\\^", TK_BIT_XOR},            // bitwise XOR
    {"0[xX][0-9a-fA-F]+", TK_NUM},  // hexadecimal number
    {"[0-9]+", TK_NUM},             // decimal number
    {"\\$[a-zA-Z0-9]+", TK_REG}     // register
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
    int i;
    char error_msg[128];
    int ret;

    for (i = 0; i < NR_REGEX; i++) {
        ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
        if (ret != 0) {
            regerror(ret, &re[i], error_msg, 128);
            panic("regex compilation failed: %s\n%s", error_msg,
                  rules[i].regex);
        }
    }
}

typedef struct token {
    int type;
    char str[32];
} Token;

static Token tokens[1024]
    __attribute__((used)) = {};  // Increased token array size
static int nr_token __attribute__((used)) = 0;

static bool make_token(char* e) {
    int position = 0;
    int i;
    regmatch_t pmatch;

    nr_token = 0;

    while (e[position] != '\0') {
        /* Try all rules one by one. */
        for (i = 0; i < NR_REGEX; i++) {
            if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 &&
                pmatch.rm_so == 0) {
                char* substr_start = e + position;
                int substr_len = pmatch.rm_eo;

                Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
                    i, rules[i].regex, position, substr_len, substr_len,
                    substr_start);

                position += substr_len;

                /* Record the token in the array `tokens` */
                switch (rules[i].token_type) {
                    case TK_NOTYPE:
                        break;  // Skip spaces
                    case TK_NUM:
                    case TK_REG:
                    default:
                        tokens[nr_token].type = rules[i].token_type;
                        if (substr_len > 31)
                            substr_len = 31;  // Prevent overflow
                        strncpy(tokens[nr_token].str, substr_start, substr_len);
                        tokens[nr_token].str[substr_len] = '\0';
                        nr_token++;
                }
                break;
            }
        }

        if (i == NR_REGEX) {
            printf("no match at position %d\n%s\n%*.s^\n", position, e,
                   position, "");
            return false;
        }
    }

    /* Post-process tokens to identify special operators */
    for (i = 0; i < nr_token; i++) {
        // Identify deref operations: * at the beginning or after an operator or
        // parenthesis
        if (tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type != TK_NUM &&
                                                 tokens[i - 1].type != TK_REG &&
                                                 tokens[i - 1].type != ')'))) {
            tokens[i].type = TK_DEREF;
        }

        // Identify negative sign: - at the beginning or after an operator or
        // parenthesis
        if (tokens[i].type == '-' && (i == 0 || (tokens[i - 1].type != TK_NUM &&
                                                 tokens[i - 1].type != TK_REG &&
                                                 tokens[i - 1].type != ')'))) {
            tokens[i].type = TK_NEGATIVE;
        }
    }

    return true;
}

// Check if the parentheses in an expression are matched
static bool check_parentheses(int p, int q) {
    if (tokens[p].type != '(' || tokens[q].type != ')') {
        return false;
    }

    int count = 0;
    for (int i = p; i <= q; i++) {
        if (tokens[i].type == '(')
            count++;
        else if (tokens[i].type == ')')
            count--;

        // If count becomes 0 before the end, the expression is not surrounded
        // by a matched pair
        if (count == 0 && i < q)
            return false;
    }

    return count == 0;
}

// Find the main operator in the expression with proper precedence handling
static int find_main_op(int p, int q) {
    int op_pos = -1;
    int op_priority = 0;
    int bracket_count = 0;

    // Define operator precedence (higher number = lower precedence)
    for (int i = p; i <= q; i++) {
        // Skip tokens in parentheses
        if (tokens[i].type == '(') {
            bracket_count++;
            continue;
        }
        if (tokens[i].type == ')') {
            bracket_count--;
            continue;
        }
        if (bracket_count > 0) {
            continue;  // Skip tokens inside brackets
        }

        // Set priority based on operator type
        int current_priority = 0;
        switch (tokens[i].type) {
            case TK_OR:
                current_priority = 1;  // Lowest priority
                break;
            case TK_AND:
                current_priority = 2;
                break;
            case TK_BIT_OR:
                current_priority = 3;
                break;
            case TK_BIT_XOR:
                current_priority = 4;
                break;
            case TK_BIT_AND:
                current_priority = 5;
                break;
            case TK_EQ:
            case TK_NEQ:
                current_priority = 6;
                break;
            case '+':
            case '-':
                current_priority = 7;
                break;
            case '*':
            case '/':
                current_priority = 8;
                break;
            case TK_DEREF:
            case TK_NEGATIVE:
                current_priority = 9;  // Highest priority (process last)
                break;
            default:
                current_priority = 0;  // Not an operator
        }

        // Update the main operator if current has lower or equal priority
        // For equal priority operators, we choose the leftmost one to ensure
        // left-to-right evaluation (except for unary operators which are
        // right-to-left)
        if (current_priority > 0 &&
            (op_pos == -1 || current_priority <= op_priority)) {
            op_pos = i;
            op_priority = current_priority;
        }
    }

    return op_pos;
}

// Evaluate an expression recursively
static word_t eval(int p, int q, bool* success) {
    if (p > q) {
        // Empty expression
        *success = false;
        return 0;
    } else if (p == q) {
        // Single token - must be a number or register
        word_t val = 0;
        switch (tokens[p].type) {
            case TK_NUM:
                if (tokens[p].str[0] == '0' &&
                    (tokens[p].str[1] == 'x' || tokens[p].str[1] == 'X')) {
                    // Hexadecimal number
                    sscanf(tokens[p].str, "%x", &val);
                } else {
                    // Decimal number
                    sscanf(tokens[p].str, "%u", &val);
                }
                break;
            case TK_REG:
                // Remove $ from register name and get its value
                val = isa_reg_str2val(tokens[p].str + 1, success);
                if (!*success) {
                    printf("Invalid register name: %s\n", tokens[p].str);
                    return 0;
                }
                break;
            default:
                *success = false;
                printf("Invalid single token: %d\n", tokens[p].type);
                return 0;
        }
        return val;
    } else if (check_parentheses(p, q)) {
        // Expression surrounded by matched parentheses - evaluate the inner
        // expression
        return eval(p + 1, q - 1, success);
    } else {
        // Find the main operator
        int op = find_main_op(p, q);
        if (op == -1) {
            *success = false;
            printf("Failed to find main operator between positions %d and %d\n",
                   p, q);
            return 0;
        }

        // Handle unary operators (TK_DEREF and TK_NEGATIVE)
        if (tokens[op].type == TK_DEREF) {
            word_t addr = eval(op + 1, q, success);
            if (!*success) {
                return 0;
            }
            // Dereference the address to get a 32/64-bit value from memory
            return vaddr_read(addr, sizeof(word_t));
        }

        if (tokens[op].type == TK_NEGATIVE) {
            word_t val = eval(op + 1, q, success);
            if (!*success) {
                return 0;
            }
            return -val;
        }

        // For binary operators, evaluate both sides
        word_t val1 = eval(p, op - 1, success);
        if (!*success) {
            return 0;
        }

        word_t val2 = eval(op + 1, q, success);
        if (!*success) {
            return 0;
        }

        // Perform the operation based on operator type
        switch (tokens[op].type) {
            case '+':
                return val1 + val2;
            case '-':
                return val1 - val2;
            case '*':
                return val1 * val2;
            case '/':
                if (val2 == 0) {
                    printf("Division by zero\n");
                    *success = false;
                    return 0;
                }
                return val1 / val2;
            case TK_EQ:
                return val1 == val2;
            case TK_NEQ:
                return val1 != val2;
            case TK_AND:
                return val1 && val2;
            case TK_OR:
                return val1 || val2;
            case TK_BIT_AND:
                return val1 & val2;
            case TK_BIT_OR:
                return val1 | val2;
            case TK_BIT_XOR:
                return val1 ^ val2;
            default:
                printf("Unknown operator: %d\n", tokens[op].type);
                *success = false;
                return 0;
        }
    }
}

word_t expr(char* e, bool* success) {
    if (!make_token(e)) {
        *success = false;
        return 0;
    }

    /* Process the tokens to identify dereference and negative operators */
    for (int i = 0; i < nr_token; i++) {
        // Identify dereference operations: * at beginning or after non-value
        // token
        if (tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type != TK_NUM &&
                                                 tokens[i - 1].type != TK_REG &&
                                                 tokens[i - 1].type != ')'))) {
            tokens[i].type = TK_DEREF;
        }

        // Identify negative sign: - at beginning or after non-value token
        if (tokens[i].type == '-' && (i == 0 || (tokens[i - 1].type != TK_NUM &&
                                                 tokens[i - 1].type != TK_REG &&
                                                 tokens[i - 1].type != ')'))) {
            tokens[i].type = TK_NEGATIVE;
        }
    }

    /* Evaluate the expression */
    *success = true;
    return eval(0, nr_token - 1, success);
}