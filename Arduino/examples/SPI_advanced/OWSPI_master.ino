#include <OneWire.h>
OneWire ow(10); // DQ on pin 10, You also need 1k resistor between DQ and VCC

#include <DS2450.h>
DS2450 pex(&ow);

byte addr[] = {0x20,0x41,0x42,0x0F,0x48,0x4E,0x59,0x49};
#define PEX_SS_PIN 1

// == simple SPI-master library (begin) ==

void spi_init(uint8_t *addr, uint8_t ss) {
  pex.pinMode(addr, ss, OUTPUT, HIGH);
}
void spi_select(uint8_t *addr, uint8_t ss) {
  pex.digitalWrite(addr, ss, LOW);
  pex.spi_start(NULL);
}
void spi_unselect(uint8_t *addr, uint8_t ss) {
  pex.digitalWrite(addr, ss, HIGH);
}
uint8_t spi_get(uint16_t a) {
  pex.spi_transfer_msbfirst(0x0f);
  pex.spi_transfer_msbfirst(a >> 8);
  pex.spi_transfer_msbfirst(a & 0xff);
  return pex.spi_transfer_msbfirst(0x00);
}
void spi_put(uint16_t a, uint8_t v) {
  pex.spi_transfer_msbfirst(0xf0);
  pex.spi_transfer_msbfirst(a >> 8);
  pex.spi_transfer_msbfirst(a & 0xff);
  pex.spi_transfer_msbfirst(v);
}

// == simple SPI-master library (end) ==

void setup(void) {
  Serial.begin(9600);
  spi_init(addr, PEX_SS_PIN);
}

void loop(void) {
  uint8_t i;

  if (pex.alarmPorIsSet(addr)) {
    pex.alarmPorClear(addr);
    spi_init(addr, PEX_SS_PIN);
  }

  spi_select(addr, PEX_SS_PIN);
  spi_put(2, 2); // set D2 mode to output-low
  spi_unselect(addr, PEX_SS_PIN);
  delay(500);

  spi_select(addr, PEX_SS_PIN);
  spi_put(2, 3); // set D2 mode to output-high
  for (i = 0; i < 8; i++) {
    Serial.print("ADC");
    Serial.print(i);
    Serial.print(" = ");
    Serial.println(spi_get(100 + i)); // memory array
  }
  spi_unselect(addr, PEX_SS_PIN);
  delay(500);
}

