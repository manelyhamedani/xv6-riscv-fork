#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/spinlock.h"
#include "kernel/riscv.h"
#include "kernel/proc.h"
#include "user/user.h"
#include <stddef.h>


int main() {
    if (fork() == 0) {
        for (int i = 0; i < 1000000000; i++) {

        }
        set_cpu_quota(getpid(), 3);
        for (int i = 0; i < 1000000000; i++) {

        }
        printf("I got quota limit! But my total cpu usage is %u\n", cpu_usage());
        return 0;
    }
    if (deadfork(10) == 0) {
        // 6 tick
        for (int i = 0; i < 1000000000; i++) {

        }
        printf("I live longer!\n");
        // 6 tick
        for (int i = 0; i < 2000000000; i++) {

        }
        printf("I am dead!\n");     // should not print if ttl <= 12
        return 0;
    }
    if (deadfork(8) == 0) {
        for (int i = 0; i < 1000000000; i++) {

        }
        printf("soon to be dead!\n");
        return 0;
    }
    if (fork() == 0) {
        sleep(6);
        printf("I had less cpu usage!\n");
        return 0;
    }
    sleep(30);
    char *argv[] = {"top", 0};
    exec("top", argv);
    printf("exec failed!\n");

    return 0;
}