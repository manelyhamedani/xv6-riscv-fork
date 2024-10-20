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
    
    struct child_processes *cp = malloc(sizeof(struct child_processes));
    int xstat = child_processes(cp);
    if (xstat != 0) {
        return xstat;
    }

    printf("number of child: %d\n", cp->count);
    printf("PID\t\tPPID\t\tSTATE\t\tNAME\n");
    for (int i = 0; i < cp->count; i++) {
        if (cp->processes[i].state == SLEEPING || cp->processes[i].state == RUNNABLE) {
            printf("%d\t\t%d\t\t%s\t%s\n", cp->processes[i].pid, cp->processes[i].ppid, p_state[cp->processes[i].state], cp->processes[i].name);
        }
        else {
            printf("%d\t\t%d\t\t%s\t\t%s\n", cp->processes[i].pid, cp->processes[i].ppid, p_state[cp->processes[i].state], cp->processes[i].name);
        }
    }

    free(cp);
    return 0;
}
