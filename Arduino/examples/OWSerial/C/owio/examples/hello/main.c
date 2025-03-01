#include <owio.h>
#include <util/delay.h>

int main() {
  owio_start();
  for (;;) {
    printf("Hello, World!\n");
    _delay_ms(1000);
  }
}
