#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/spinlock.h"
#include "kernel/riscv.h"
#include "kernel/proc.h"
#include "user/user.h"

volatile int a = 0, b = 0, c = 0;

void *my_thread(void *arg) {
    // int *number = arg;

    // for (int i = 0; i < 100; ++i) {
    //     (*number)++;

    //     if (number == &a) {
    //         printf("thread a: %d\n", *number);
    //     }
    //     else if (number == &b) {
    //         printf("thread b: %d\n", *number);
    //     }
    //     else if (number == &c) {
    //         printf("thread c: %d\n", *number);
    //     }
    // }
    printf("runner function of thread %p\n", (int *) arg);
    return 0;
}

int main() {
    int amem[100], bmem[100], cmem[100];

    struct stack astack, bstack, cstack;

    astack.mem = (uint64) &amem[0];
    astack.size = sizeof(amem);

    bstack.mem = (uint64) &bmem[0];
    bstack.size = sizeof(bmem);

    cstack.mem = (uint64) &cmem[0];
    cstack.size = sizeof(cmem);

    int ta = create_thread(my_thread, (void *) &a, (struct stack *) &astack);
    int tb = create_thread(my_thread, (void *) &b, (struct stack *) &bstack);
    int tc = create_thread(my_thread, (void *) &c, (struct stack *) &cstack);

    // while(1) {
    //     // printf("while\n");
    // }
    printf("joining on %d\n", ta);
    int rv = join_thread(ta);
    printf("join rv: %d\n", rv);
    printf("joined to ta finished\n");
    join_thread(tb);
    printf("joined to tb finished\n");
    join_thread(tc);
    printf("joined to tc finished\n");

    exit(0);

}

