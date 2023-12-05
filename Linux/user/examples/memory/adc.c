#include <stdio.h>
#include <unistd.h>
#include "ds2450.h"

int main() {
    ds2450_t pex = ds2450_new("20-594e480f4241");

    for (;;) {
        unsigned short adc[4];
        ds2450_analogConvert(pex, 0, 2, 3, 4);
        usleep(8*1000);
        ds2450_analogReadLastAll(pex, adc);
        printf("ADC = % 5u % 5u % 5u % 5u\n",
               adc[0], adc[1], adc[2], adc[3]);
        usleep(100*1000);
    }
}

