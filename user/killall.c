#include "kernel/param.h"
#include "kernel/types.h"
#include "user.h"

int main() {
    for (int i = 3; i < NPROC; ++i) {
        kill(i);
    }
    return 0;
}