#include <OneWire.h>
OneWire ow(7); // DQ on pin 7, You also need 1k resistor between DQ and VCC

#include <DS2450.h>
DS2450 pex(&ow);

byte addr[] = {0x20,0x41,0x42,0x0F,0x48,0x4E,0x59,0x49};
#define PEX_RST_PIN 0
#define PEX_SS_PIN  1

void setup(void) {
  Serial.begin(19200);
  pex.pinMode(addr, PEX_SS_PIN, INPUT_PULLUP);
  pex.pinMode(NULL, PEX_RST_PIN, OUTPUT, LOW);
  delay(20); // reset pulse
  pex.pinMode(NULL, PEX_RST_PIN, INPUT_PULLUP);
  delay(1500); // let it boot
  pex.pinMode(NULL, 2, OUTPUT, LOW); // SCK
  pex.pinMode(NULL, PEX_SS_PIN, OUTPUT, LOW);
  pex.spi_start(NULL);
}

void loop(void) {
  static uint8_t iesc = 0, oesc = 0;
  uint8_t c = 0xff;
  if (Serial.available()) {
    c = Serial.peek();
    if (oesc || (c != 0x0f && c != 0xff)) {
      c = Serial.read();
      oesc = 0;
    } else {
      oesc = 1;
      c = 0x0f;
    }
  }
  c = pex.spi_transfer_lsbfirst(c);
  if (iesc || (c != 0x0f && c != 0xff)) {
    Serial.write(c);
    iesc = 0;
  } else if (c == 0x0f) {
    iesc = 1;
  }
}

