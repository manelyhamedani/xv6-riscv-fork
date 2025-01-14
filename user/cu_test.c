#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/spinlock.h"
#include "kernel/riscv.h"
#include "kernel/proc.h"
#include "user/user.h"


int main() {
    uint cu = cpu_usage();
    printf("first cu: %u\n", cu);
    sleep(20);
    uint start_tick = uptime();
    for (int i = 0; i < 1000000000; ++i) {
        
    }
    uint end_tick = uptime();
    cu = cpu_usage();
    printf("second cu: %u\n", cu);
    printf("diff: %u\n", end_tick - start_tick);
    return 0;
}