#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/spinlock.h"
#include "kernel/riscv.h"
#include "kernel/proc.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() { 
    struct top *t = malloc(sizeof(struct top));
    int xstat = top(t);
    if (xstat != 0) {
        return xstat;
    }

    printf("number of processes: %d\n", t->count);
    printf("PID        PPID        STATE        NAME        START        USAGE\n");
    for (int i = 0; i < t->count; i++) {
        printf("%d", t->processes[i].pid);
        adjust_int(t->processes[i].pid, 11);

        printf("%d", t->processes[i].ppid);
        adjust_int(t->processes[i].ppid, 12);

        printf("%s", p_state[t->processes[i].state]);
        adjust_str(p_state[t->processes[i].state], 13);

        printf("%s", t->processes[i].name);
        adjust_str(t->processes[i].name, 12);

        printf("%d", t->processes[i].cpu_usage.start_tick);
        adjust_int(t->processes[i].cpu_usage.start_tick, 13);

        printf("%d\n", t->processes[i].cpu_usage.sum_of_ticks);
    }
    
    free(t);
    return 0;
}