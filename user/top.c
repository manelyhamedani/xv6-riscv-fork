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
    struct top *t = malloc(sizeof(struct top));
    int xstat = top(t);
    if (xstat != 0) {
        return xstat;
    }

    printf("number of processes: %d\n", t->count);
    printf("PID\t\tPPID\t\tSTATE\t\tNAME\t\tSTART\t\tUSAGE\n");
    for (int i = 0; i < t->count; i++) {
        // column fixing
        if (t->processes[i].state == SLEEPING || t->processes[i].state == RUNNABLE) {
            printf("%d\t\t%d\t\t%s\t%s\t\t%u\t\t%u\n", 
                    t->processes[i].pid, 
                    t->processes[i].ppid, 
                    p_state[t->processes[i].state], 
                    t->processes[i].name,
                    t->processes[i].cpu_usage.start_tick,
                    t->processes[i].cpu_usage.sum_of_ticks);
        }
        else {
            printf("%d\t\t%d\t\t%s\t\t%s\t\t%u\t\t%u\n", 
                    t->processes[i].pid, 
                    t->processes[i].ppid, 
                    p_state[t->processes[i].state], 
                    t->processes[i].name,
                    t->processes[i].cpu_usage.start_tick,
                    t->processes[i].cpu_usage.sum_of_ticks);
        }
    }
    
    free(t);
    return 0;
}