#include <stdio.h>
#include <unistd.h>
#include "ds2450.h"

int main() {
    ds2450_t pex = ds2450_new("20-594e480f4241");
    int i;

    for (;;) {
        for (i = 0; i < 255; i++) {
            ds2450_analogWrite(pex, 1, i);
            usleep(1*1000);
        }
        for (i = 255; i > 0; i--) {
            ds2450_analogWrite(pex, 1, i);
            usleep(1*1000);
        }
    }
}

