#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/spinlock.h"
#include "kernel/riscv.h"
#include "kernel/proc.h"
#include "user/user.h"


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Scheduler not specified!\n");
        return -1;
    }

    if (strcmp("RR", argv[1]) == 0) {
        csched(RR);
    }
    else if (strcmp("MinCU", argv[1]) == 0) {
        csched(MinCU);
    }
    else {
        printf("Scheduler invalid!\n");
        return -1;
    }
}
