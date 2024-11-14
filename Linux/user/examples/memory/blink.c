#include <stdio.h>
#include <unistd.h>
#include "ds2450.h"

int main() {
    ds2450_t pex = ds2450_new("20-594e480f4241");
    int pin = 0;

    for (;;) {
        ds2450_pinMode(pex, pin, OUTPUT|HIGH);
        usleep(500*1000);
        ds2450_pinMode(pex, pin, OUTPUT|LOW);
        usleep(500*1000);
    }
}

