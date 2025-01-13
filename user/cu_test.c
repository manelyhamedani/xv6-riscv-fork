#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/spinlock.h"
#include "kernel/riscv.h"
#include "kernel/proc.h"
#include "user/user.h"


int main() {
    uint cu = cpu_usage();
    printf("first cu: %u\n", cu);
    sleep(100);
    cu = cpu_usage();
    printf("second cu: %u\n", cu);
    sleep(10);
    cu = cpu_usage();
    printf("third cu: %u\n", cu);
    return 0;
}