// === simple SPI-slave library (begin) ===

volatile uint8_t mode;
uint16_t addr;
typedef uint8_t (*readf_t)(uint16_t);
typedef void (*writef_t)(uint16_t, uint8_t);
readf_t readf;
writef_t writef;

// communication protocol over SPI:
// 1) read value from addr
//    MOSI: 0x0f [addr_h] [addr_l] 0x00
//    MISO: 0x00 0x00     0x00     [value]
// 2) write value to addr
//    MOSI: 0xf0 [addr_h] [addr_l] [value]
//    MISO: 0x00 0x00     0x00     0x00
ISR(SPI_STC_vect) {
  uint8_t i = SPDR;
  switch (mode) {
  case 0:
    if (i == 0x0f) mode = 1;
    if (i == 0xf0) mode = 4;
    break;
  case 1:
  case 4:
    addr = i << 8;
    mode++;
    break;
  case 2:
    SPDR = (*readf)(addr | i);
    mode++;
    break;
  case 5:
    addr |= i;
    mode++;
    break;
  case 6:
    (*writef)(addr, i);
  default:
    mode = 0;
  }
}

void spi_init(readf_t rf, writef_t wf) {
  mode = 0;
  readf = rf;
  writef = wf;
  pinMode(MISO, OUTPUT);
  SPCR = _BV(SPE) | _BV(SPIE);
}

// === simple SPI-slave library (end) ===

void digitalMode(uint8_t p, uint8_t m) {
  switch (m) {
  default:
    pinMode(p, INPUT);
    break;
  case 1:
    pinMode(p, INPUT_PULLUP);
    break;
  case 2:
    pinMode(p, OUTPUT);
    digitalWrite(p, LOW);
    break;
  case 3:
    pinMode(p, OUTPUT);
    digitalWrite(p, HIGH);
  }
}

// 0..7 read-only ADC results
// 8..15 read/write memory
uint8_t storage[16];

// map virtual addresses to something real
uint8_t get(uint16_t a) {
  if (a >= 2 && a < 10)
    return digitalRead(a);
  if (a >= 100 && a < 100 + sizeof(storage))
    return storage[a - 100];
  return 0;
}
void put(uint16_t a, uint8_t v) {
  if (a >= 2 && a < 10)
    digitalMode(a, v);
  if (a >= 108 && a < 100 + sizeof(storage))
    storage[a - 100] = v;
}

void setup (void) {
  spi_init(&get, &put);
}

void loop (void) {
  uint8_t i;
  for (i = 0; i < 8; i++)
    storage[i] = analogRead(i) / 4;
}

