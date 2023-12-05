#include <OneWire.h>
OneWire ow(7); // DQ on pin 7, You also need 1k resistor between DQ and VCC

#include <DS2450.h>
DS2450 pex(&ow);

byte addr[] = {0x20,0x41,0x42,0x0F,0x48,0x4E,0x59,0x49};
#define pin 0

void setup(void) {
  pex.pinMode(addr, pin, OUTPUT);
}

void loop(void) {
  if (pex.alarmPorIsSet(addr)) {
    pex.alarmPorClear(addr);
    pex.pinMode(addr, pin, OUTPUT);
  }
  pex.digitalWrite(addr, pin, HIGH);
  delay(500);
  pex.digitalWrite(addr, pin, LOW);
  delay(500);
}

