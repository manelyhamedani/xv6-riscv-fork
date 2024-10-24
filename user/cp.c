#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/spinlock.h"
#include "kernel/riscv.h"
#include "kernel/proc.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {
    if (fork() == 0) {
        for (int i = 0; i < 4; i++) {
            fork();
        }
        sleep(200000);
        // for (int i = 0; i < 4000000000; i++) {
        //     int x = i * i * i;
        // }
        return 0;
    }
    else {
        sleep(2);
    }
    return print_child_processes();
}
