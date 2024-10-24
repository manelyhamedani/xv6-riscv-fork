#ifndef TRAP_H
#define TRAP_H

#include "types.h"

#define REPORT_BUFFER_SIZE  10

struct report {
    char pname[16];
    int pid;
    int ancestors[NPROC];
    int ancestors_count;
    uint64 scause;
    uint64 sepc;
    uint64 stval;
};

extern struct internal_report_list {
    struct report reports[REPORT_BUFFER_SIZE];
    int count;
    int write_index;
} _internal_report_list;

struct report_traps {
    struct report reports[REPORT_BUFFER_SIZE];
    int count;
};

#endif