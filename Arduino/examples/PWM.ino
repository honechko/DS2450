#include <OneWire.h>
OneWire ow(7); // DQ on pin 7, You also need 1k resistor between DQ and VCC

#include <DS2450.h>
DS2450 pex(&ow);

byte addr[] = {0x20,0x41,0x42,0x0F,0x48,0x4E,0x59,0x49};

void setup(void) {
}

void loop(void) {
  byte i;
  for (i = 0; i < 255; i++) {
    pex.analogWrite(addr, 1, i);
    delay(10);
  }
  for (i = 255; i > 0; i--) {
    pex.analogWrite(addr, 1, i);
    delay(10);
  }
}

