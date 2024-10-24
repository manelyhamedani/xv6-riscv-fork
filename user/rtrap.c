#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/spinlock.h"
#include "kernel/riscv.h"
#include "kernel/trap.h"
#include "kernel/proc.h"
#include "kernel/stat.h"
#include "user/user.h"

struct proc* myproc(void);

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
                "csrw mepc, ra\n"         // Set the next instruction address to return to
                "csrw mtvec, t0\n"        // Set the trap vector (trigger the page fault)
                "mret\n"                  // Return from exception (this will invoke the trap)
            );
        }
    }
    else {
        sleep(20);
        print_child_processes();
        struct report_traps *rt = malloc(sizeof(struct report_traps));
        int xs = report_traps(rt);
        if (xs != 0) {
            return -1;
        }

        int count = rt->count;
        printf("report count: %d\n", count);

        printf("PID\t\tPNAME\t\tscause\t\tsepc\t\tstval\n");
        for (int i = 0; i < count; ++i) {
            printf("%d\t\t%s\t\t%lu\t\t%lu\t\t%lu\n",
                rt->reports[i].pid,
                rt->reports[i].pname,
                rt->reports[i].scause,
                rt->reports[i].sepc,
                rt->reports[i].stval
            );
        }
    }

    return 0;
}