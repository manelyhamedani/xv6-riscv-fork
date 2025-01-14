#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/spinlock.h"
#include "kernel/riscv.h"
#include "kernel/proc.h"
#include "kernel/stat.h"
#include "user/user.h"

int print_child_processes() {
    struct child_processes *cp = malloc(sizeof(struct child_processes));
    int xstat = child_processes(cp);
    if (xstat != 0) {
        return xstat;
    }

    printf("number of children: %d\n", cp->count);
    printf("PID        PPID        STATE        NAME\n");
    for (int i = 0; i < cp->count; i++) {
        printf("%d", cp->processes[i].pid);
        adjust_int(cp->processes[i].pid, 11);

        printf("%d", cp->processes[i].ppid);
        adjust_int(cp->processes[i].ppid, 12);

        printf("%s", p_state[cp->processes[i].state]);
        adjust_str(p_state[cp->processes[i].state], 13);

        printf("%s\n", cp->processes[i].name);
    }

    free(cp);
    return 0;
}

int main() {
    return print_child_processes();
}