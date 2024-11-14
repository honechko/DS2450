#include <OneWire.h>
OneWire ow(7); // DQ on pin 7, You also need 1k resistor between DQ and VCC

#include <DS2450.h>
DS2450 pex(&ow);

byte addr[] = {0x20,0x41,0x42,0x0F,0x48,0x4E,0x59,0x49};
#define PEX_SS_PIN 1

// == simple OW Remote Call master library (begin) ==

void owrcall_select(uint8_t *addr, uint8_t ss) {
  pex.pinMode(addr, ss, OUTPUT, LOW);
  pex.spi_start(NULL);
}
void owrcall_unselect(uint8_t *addr, uint8_t ss) {
  pex.pinMode(addr, ss, INPUT);
}
// execute callback function on remote side
uint8_t owrcall_get(uint16_t a) {
  pex.spi_transfer_lsbfirst(0x0f);
  pex.spi_transfer_lsbfirst(a >> 8);
  pex.spi_transfer_lsbfirst(a & 0xff);
  return pex.spi_transfer_lsbfirst(0x00);
}
// execute callback function on remote side
void owrcall_put(uint16_t a, uint8_t v) {
  pex.spi_transfer_lsbfirst(0xf0);
  pex.spi_transfer_lsbfirst(a >> 8);
  pex.spi_transfer_lsbfirst(a & 0xff);
  pex.spi_transfer_lsbfirst(v);
}

// == simple OW Remote Call master library (end) ==

void setup(void) {
  Serial.begin(9600);
}

void loop(void) {
  uint8_t i;

  owrcall_select(addr, PEX_SS_PIN);
  owrcall_put(2, 2); // set pin 2 mode to output-low
  owrcall_unselect(addr, PEX_SS_PIN);
  delay(500);

  owrcall_select(addr, PEX_SS_PIN);
  owrcall_put(2, 3); // set pin 2 mode to output-high
  for (i = 0; i < 8; i++) {
    Serial.print("ADC");
    Serial.print(i);
    Serial.print(" = ");
    Serial.println(owrcall_get(100 + i)); // memory cells
  }
  owrcall_unselect(addr, PEX_SS_PIN);
  delay(500);
}

