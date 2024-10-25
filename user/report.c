#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/trap.h"
#include "kernel/proc.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"


int main() {
    struct report_traps *rt = malloc(sizeof(struct report_traps));
    int xs = report(rt);
    if (xs != 0) {
        return -1;
    }

    int count = rt->count;
    printf("report count: %d\n", count);

    printf("PID\t\tPNAME\t\tscause\t\tsepc\t\tstval\n");
    for (int i = 0; i < count; ++i) {
        printf("%d\t\t%s\t\t0x%lx\t\t0x%lx\t\t0x%lx\n",
            rt->reports[i].pid,
            rt->reports[i].pname,
            rt->reports[i].scause,
            rt->reports[i].sepc,
            rt->reports[i].stval
        );
    }
    return 0;
}