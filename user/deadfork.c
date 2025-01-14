#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/spinlock.h"
#include "kernel/riscv.h"
#include "kernel/proc.h"
#include "user/user.h"
#include <stddef.h>


int main() {
    if (deadfork(10) == 0) {
        // for (int i = 0; i < 4000000000; i++) {
        //     printf("%d\n", i);
        // }
        printf("tick 1\n");
        sleep(1);
        printf("tick 2\n");
        sleep(1);
        printf("tick 3\n");
        sleep(1);
        printf("tick 4\n");
        sleep(1);
        printf("tick 5\n");
        sleep(1);
        printf("tick 6\n");
        sleep(1);
        printf("tick 7\n");
        sleep(1);
        printf("tick 8\n");
        sleep(1);
        printf("tick 9\n");
        sleep(1);
        printf("tick 10\n");
        sleep(1);
        printf("tick 11\n");
        sleep(1);
        printf("tick 12\n");
        sleep(1);
        return 0;
    }
    else {
        wait(NULL);
        printf("parent exit\n");
    }

    return 0;
}