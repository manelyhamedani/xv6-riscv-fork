#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/spinlock.h"
#include "kernel/riscv.h"
#include "kernel/proc.h"
#include "user/user.h"

volatile int a = 0, b = 0, c = 0;

void *my_thread(void *arg) {
    // int *number = (int *) arg;

    // for (int i = 0; i < 5; ++i) {
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
    printf("runner function of thread %p\n", (void *) arg);
    return 0;
}

int main() {
    uint64 amem[200], bmem[200], cmem[200];

    struct stack astack, bstack, cstack;

    astack.mem = (uint64) &amem[0];
    astack.size = sizeof(amem);

    bstack.mem = (uint64) &bmem[0];
    bstack.size = sizeof(bmem);

    cstack.mem = (uint64) &cmem[0];
    cstack.size = sizeof(cmem);
    // printf("astack: %lu\n", (uint64) &amem[199]);

    // printf("runner address %p\n", (void *) my_thread);
    // printf("a addres %p\n", (int *) &a);
    // printf("b addres %p\n", (int *) &b);
    // printf("c addres %p\n", (int *) &c);
    int ta = create_thread(my_thread, (void *) &a, (void *) &amem[199]);
    int tb = create_thread(my_thread, (void *) &b, (void *) &bmem[199]);
    int tc = create_thread(my_thread, (void *) &c, (void *) &cmem[199]);

    // while(1) {
    //     // printf("while\n");
    // }
    // printf("joining on %d\n", ta);
    // printf("ta: %d\n", ta);
    int rv = join_thread(ta);
    // printf("join rv: %d\n", rv);
    // printf("joined to ta finished\n");
    join_thread(tb);
    // printf("joined to tb finished\n");
    join_thread(tc);
    // printf("joined to tc finished\n");

    exit(0);

}

