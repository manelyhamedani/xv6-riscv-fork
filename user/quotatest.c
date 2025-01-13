#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/spinlock.h"
#include "kernel/riscv.h"
#include "kernel/proc.h"
#include "user/user.h"


int main() {
    int cid;
    if ((cid = fork()) == 0) {
        sleep(20);
        return 0;
    }
    else {
        sleep(2);
        printf("set child quota output is %d\n", set_cpu_quota(cid, 10));
        printf("set self quota output is %d\n", set_cpu_quota(getpid(), 10));
        printf("set others quota output is %d\n", set_cpu_quota(0, 10));
    }
    return 0;
}