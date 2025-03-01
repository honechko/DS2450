#include <OWSerial.h>

void setup(void) {
  OWSerial.begin();
}

void loop(void) {
  OWSerial.println("Hello, World!");
  delay(1000);
}

