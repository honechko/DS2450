#include <owio.h>

int main() {
  owio_start();
  for (;;) {
    int c = getchar();
    if (c != EOF)
      putchar(c);
  }
}
