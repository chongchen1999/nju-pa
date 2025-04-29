#include "sdb.h"
#include <memory/vaddr.h>

#define NR_WP 32

typedef struct watchpoint {
    int NO;
    struct watchpoint* next;

    /* Added members for watchpoint functionality */
    char expr[256];    // Expression string
    uint32_t old_val;  // Previous value
    bool enabled;      // Whether the watchpoint is enabled
} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
    int i;
    for (i = 0; i < NR_WP; i++) {
        wp_pool[i].NO = i;
        wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
        wp_pool[i].enabled = false;
        wp_pool[i].expr[0] = '\0';
    }

    head = NULL;
    free_ = wp_pool;
}

/* Implementation of watchpoint functionality */

// Allocate a new watchpoint from the free list
WP* new_wp() {
    if (free_ == NULL) {
        printf("Error: No free watchpoints available.\n");
        return NULL;
    }

    WP* wp = free_;
    free_ = free_->next;

    // Add to active watchpoints list
    wp->next = head;
    head = wp;
    wp->enabled = true;

    return wp;
}

// Free a watchpoint and return it to the free list
void free_wp(WP* wp) {
    if (wp == NULL)
        return;

    // If it's the head of the list
    if (head == wp) {
        head = wp->next;
    } else {
        // Find the watchpoint in the list
        WP* prev = head;
        while (prev && prev->next != wp) {
            prev = prev->next;
        }

        if (prev == NULL) {
            printf("Error: Watchpoint not found in active list.\n");
            return;
        }

        prev->next = wp->next;
    }

    // Add to free list
    wp->next = free_;
    free_ = wp;
    wp->enabled = false;
    wp->expr[0] = '\0';
}

// Set a new watchpoint
WP* set_watchpoint(char* expr_str) {
    if (strlen(expr_str) >= 256) {
        printf("Error: Expression too long.\n");
        return NULL;
    }

    // Evaluate the expression to get initial value
    bool success = true;
    uint32_t val = expr(expr_str, &success);

    if (!success) {
        printf("Error: Invalid expression.\n");
        return NULL;
    }

    WP* wp = new_wp();
    if (wp == NULL)
        return NULL;

    strcpy(wp->expr, expr_str);
    wp->old_val = val;

    printf("Watchpoint %d: %s (initial value = 0x%08x)\n", wp->NO, wp->expr,
           wp->old_val);

    return wp;
}

// Delete a watchpoint by number
bool delete_watchpoint(int num) {
    WP* wp = head;
    while (wp) {
        if (wp->NO == num) {
            free_wp(wp);
            printf("Deleted watchpoint %d\n", num);
            return true;
        }
        wp = wp->next;
    }

    printf("Error: No watchpoint with number %d.\n", num);
    return false;
}

// Print all watchpoints
void print_watchpoints() {
    if (head == NULL) {
        printf("No watchpoints.\n");
        return;
    }

    printf("Num    Type       Expr        Value\n");
    printf("---    ----       ----        -----\n");

    WP* wp = head;
    while (wp) {
        // Re-evaluate the expression to get the current value
        bool success = true;
        uint32_t current_val = expr(wp->expr, &success);

        if (success) {
            printf("%-7d watchpoint  %-10s  0x%08x\n", wp->NO, wp->expr,
                   current_val);
        } else {
            printf("%-7d watchpoint  %-10s  <error>\n", wp->NO, wp->expr);
        }

        wp = wp->next;
    }
}

// Check if any watchpoint has been triggered
// Returns true if program should be stopped
bool check_watchpoints() {
    bool triggered = false;
    WP* wp = head;

    while (wp) {
        if (!wp->enabled) {
            wp = wp->next;
            continue;
        }

        // Re-evaluate the expression to get current value
        bool success = true;
        uint32_t new_val = expr(wp->expr, &success);

        if (!success) {
            printf("Error: Failed to evaluate watchpoint %d expression: %s\n",
                   wp->NO, wp->expr);
            wp = wp->next;
            continue;
        }

        if (new_val != wp->old_val) {
            printf("Watchpoint %d: %s\n", wp->NO, wp->expr);
            printf("Old value = 0x%08x\n", wp->old_val);
            printf("New value = 0x%08x\n", new_val);

            wp->old_val = new_val;  // Update the old value
            triggered = true;
        }

        wp = wp->next;
    }

    return triggered;
}

// Get a watchpoint by number
WP* get_wp(int num) {
    WP* wp = head;
    while (wp) {
        if (wp->NO == num) {
            return wp;
        }
        wp = wp->next;
    }
    return NULL;
}