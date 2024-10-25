#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/trap.h"
#include "kernel/proc.h"
#include "kernel/stat.h"
#include "user/user.h"


int main() {
    int pid;
    if (fork() == 0) {
        for (int i = 0; i < 4; ++i) {
            pid = fork();
            if (pid > 0) {
                wait(0);
            }
        }
        if (pid == 0) {
            asm volatile(
                "li t0, 14\n"             // Load the trap number for page fault (T_PGFLT)
                "mret\n"                  // Return from exception (this will invoke the trap)
            );
        }
    }
    else {
        sleep(20);
        char *argv[] = {"myrep", 0};
        exec("myrep", argv);
        printf("exec failed\n");
    }
    return 0;
}