#include <OWSerial.h>

void setup(void) {
  OWSerial.begin();
}

void loop(void) {
  if (OWSerial.available())
    OWSerial.write(OWSerial.read());
}

