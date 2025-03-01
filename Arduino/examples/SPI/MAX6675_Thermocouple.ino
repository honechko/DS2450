#include <DS2450.h>
OneWire ow(7); // DQ on pin 7, You also need 1k resistor between DQ and VCC
DS2450 pex(&ow);

byte addr[] = {0x20,0x41,0x42,0x0F,0x48,0x4E,0x59,0x49};
#define CS0 0
#define CS1 1

// DS2450  MAX6675
//  GND <--> GND
//  VCC <--> VCC
//  P0
//  P1  <--> CS
//  P2  <--> SCK
//  P3
//  P4  <--> SO

uint16_t max6675_read(uint8_t *addr, uint8_t cs) {
  uint16_t d;
  pex.digitalWrite(addr, cs, LOW);
  pex.spi_start(NULL);
  d = pex.spi_transfer16_msbfirst(0);
  pex.digitalWrite(NULL, cs, HIGH);
  return !(d & 4) ? d >> 3 : 40000;
}

void setup(void) {
  Serial.begin(9600);
  pex.pinMode(addr, CS1, OUTPUT, HIGH);
}

void loop(void) {
  if (pex.alarmPorIsSet(addr)) {
    pex.alarmPorClear(addr);
    pex.pinMode(addr, CS1, OUTPUT, HIGH);
  }
  Serial.print("T = ");
  Serial.print(max6675_read(addr, CS1) / 4.0);
  Serial.println(" C");
  delay(1000);
}

