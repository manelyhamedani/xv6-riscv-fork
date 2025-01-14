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
    printf("diff: %u\n", end_tick - start_tick);
    printf("second cu: %u\n", cu);
    sleep(10);
    cu = cpu_usage();
    printf("third cu: %u\n", cu);
    return 0;
}