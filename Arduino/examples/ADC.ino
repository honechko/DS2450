#include <OneWire.h>
OneWire ow(7); // DQ on pin 7, You also need 1k resistor between DQ and VCC

#include <DS2450.h>
DS2450 pex(&ow);

byte addr[] = {0x20,0x41,0x42,0x0F,0x48,0x4E,0x59,0x49};
bool firstpass;

void setup(void) {
  Serial.begin(9600);
  firstpass = true;
}

void loop(void) {
  if (pex.alarmPorIsSet(addr) || firstpass) {
    pex.alarmPorClear(addr);
    pex.analogResolution(addr, 0, 12);
    pex.analogReference(addr, 0, DEFAULT);
    pex.analogResolution(addr, 2, 12);
    pex.analogReference(addr, 2, INTERNAL);
    pex.analogResolution(addr, 3, 13);
    pex.analogReference(addr, 3, DEFAULT);
    pex.pinMode(addr, 3, INPUT_PULLUP);
    firstpass = false;
  }
  Serial.print("Pin 0 Voltage = VCC * ");
  Serial.println((double)pex.analogRead(addr, 0) / 65536);
  Serial.print("Pin 2 Voltage = ");
  Serial.print((double)pex.analogRead(addr, 2) / 65536 * 1.1);
  Serial.println(" V");
  Serial.print("Pin 3 Resistance = ");
  // pullup is 30..40 kOhm => a = R / (35000 + R) => R = 35000 / (1/a - 1)
  Serial.print(35000 / (65536 / (double)pex.analogRead(addr, 3) - 1));
  Serial.println(" Ohm");
  delay(1000);
}

