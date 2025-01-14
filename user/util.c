#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/spinlock.h"
#include "kernel/riscv.h"
#include "kernel/proc.h"
#include "kernel/stat.h"
#include "user/user.h"

const char *p_state[] = {
    [UNUSED] = "UNUSED",
    [USED] = "USED",
    [RUNNING] = "RUNNING",
    [SLEEPING] = "SLEEPING",
    [RUNNABLE] = "RUNNABLE",
    [ZOMBIE] = "ZOMBIE"
};


void adjust_str(const char *str, int limit_size) {
    int str_len = strlen(str);
    int spaces = limit_size - str_len;
    for (int i = 0; i < spaces; i++) {
        printf(" ");
    }
}

void adjust_int(int n, int limit_size) {
    int num_len = 0;
    if (n == 0) {
        num_len = 1;
    }
    while (n > 0){
        n /= 10;
        num_len++;
    }
    int spaces = limit_size - num_len;
    for (int i = 0; i < spaces; i++) {
        printf(" ");
    }
}

void adjust_hex(int n, int limit_size) {
    int num_len = 0;
    if (n == 0) {
        num_len = 1;
    }
    while (n > 0){
        n /= 10;
        num_len++;
    }
    int spaces = limit_size - num_len - 2;
    for (int i = 0; i < spaces; i++) {
        printf(" ");
    }
}