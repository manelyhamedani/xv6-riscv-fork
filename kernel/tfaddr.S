#include "riscv.h"
#include "memlayout.h"

.globl tfadd
tfadd:
    li t6, TRAPFRAME