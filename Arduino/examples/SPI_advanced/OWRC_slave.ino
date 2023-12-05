// === simple OW Remote Call slave library (begin) ===

// communication protocol over SPI:
// 1) master gets value from addr
//    MOSI: 0x0f [addr_h] [addr_l] 0x00
//    MISO: 0x00 0x00     0x00     [value]
// 2) master puts value to addr
//    MOSI: 0xf0 [addr_h] [addr_l] [value]
//    MISO: 0x00 0x00     0x00     0x00
typedef uint8_t (*owrc_getf_t)(uint16_t);
typedef void (*owrc_putf_t)(uint16_t, uint8_t);
owrc_getf_t owrc_getf;
owrc_putf_t owrc_putf;
uint8_t owrc_mode;
uint16_t owrc_addr;
ISR(SPI_STC_vect) {
  uint8_t i = SPDR;
  switch (owrc_mode) {
  case 0:
    if (i == 0x0f) owrc_mode = 1;
    if (i == 0xf0) owrc_mode = 4;
    break;
  case 1:
  case 4:
    owrc_addr = i << 8;
    owrc_mode++;
    break;
  case 2:
    SPDR = (*owrc_getf)(owrc_addr | i);
    owrc_mode++;
    break;
  case 5:
    owrc_addr |= i;
    owrc_mode++;
    break;
  case 6:
    (*owrc_putf)(owrc_addr, i);
  default:
    owrc_mode = 0;
  }
}
void owrcalls_begin(owrc_getf_t gf, owrc_putf_t pf) {
  owrc_mode = 0;
  owrc_getf = gf;
  owrc_putf = pf;
  pinMode(MISO, OUTPUT);
  pinMode(SS, INPUT_PULLUP);
  SPCR = _BV(SPE) | _BV(SPIE) | _BV(DORD);
}
void owrcalls_end() {
  SPCR = 0;
  pinMode(MISO, INPUT);
  pinMode(SS, INPUT);
}

// === simple OW Remote Call slave library (end) ===

// some user function
void digitalSet(uint8_t p, uint8_t m) {
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

// some storage
// 0..7 read-only ADC results
// 8..15 read/write memory
uint8_t storage[16];

// define callback functions for remote get and put
uint8_t get(uint16_t a) {
  // map addresses 2..9 to Arduino pins 2..9
  if (a >= 2 && a < 10)
    return digitalRead(a);
  // map addresses 100..115 to storage cells 0..15
  if (a >= 100 && a < 100 + sizeof(storage))
    return storage[a - 100];
  return 0;
}
void put(uint16_t a, uint8_t v) {
  // map addresses 2..9 to Arduino pins 2..9
  if (a >= 2 && a < 10)
    digitalSet(a, v);
  // map addresses 108..115 to storage cells 8..15
  if (a >= 108 && a < 100 + sizeof(storage))
    storage[a - 100] = v;
}

void setup (void) {
  owrcalls_begin(&get, &put);
}

void loop (void) {
  uint8_t i;
  // fill storage cells 0..8
  for (i = 0; i < 8; i++)
    storage[i] = analogRead(i) / 4;
}

