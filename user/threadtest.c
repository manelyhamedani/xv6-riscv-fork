#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/spinlock.h"
#include "kernel/riscv.h"
#include "kernel/proc.h"
#include "user/user.h"

volatile int a = 0, b = 0, c = 0;

void *my_thread(void *arg) {
    // int *number = (int *) arg;

    // for (int i = 0; i < 1000000; ++i) {
    //     (*number)++;

        // if (number == &a) {
        //     printf("thread a: %d\n", *number);
        // }
        // else if (number == &b) {
        //     printf("thread b: %d\n", *number);
        // }
        // else if (number == &c) {
        //     printf("thread c: %d\n", *number);
        // }
    // }
    printf("runner function arg is %d\n", *((int *) arg));
    return 0;
}

int main() {
    uint64 amem[200], bmem[200], cmem[200];

    int ta = create_thread(my_thread, (void *) &a, (void *) &amem[199]);
    // printf("waiting\n");
    stop_thread(ta);
    ta = create_thread(my_thread, (void *) &a, (void *) &amem[199]);
    int tb = create_thread(my_thread, (void *) &b, (void *) &bmem[199]);
    int tc = create_thread(my_thread, (void *) &c, (void *) &cmem[199]);

    for (int i = 0; i < 10; ++i) {
        a++;
    }
    // while (1)
    // {
    //     // printf("wait\n");
    // }
    
    join_thread(ta);
    join_thread(tb);
    join_thread(tc);

    exit(0);

}

