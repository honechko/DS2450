#include <avr/interrupt.h>
#include <Arduino.h>
#include <SPI.h>
#include <OWSerial.h>

void OWSerialClass::_interrupt(void) {
  uint8_t c = SPDR;
  if (_rx_escape || (c != 0x0f && c != 0xff)) {
    uint8_t i = OWS_RX_RING(_rx_buffer_head + 1);
    if (i != _rx_buffer_tail) {
      _rx_buffer[_rx_buffer_head] = c;
      _rx_buffer_head = i;
    }
    _rx_escape = 0;
  } else if (c == 0x0f) {
    _rx_escape = 1;
  }
  c = 0xff;
  if (_tx_buffer_head != _tx_buffer_tail) {
    c = _tx_buffer[_tx_buffer_tail];
    if (_tx_escape || (c != 0x0f && c != 0xff)) {
      _tx_buffer_tail = OWS_TX_RING(_tx_buffer_tail + 1);
      _tx_escape = 0;
    } else {
      _tx_escape = 1;
      c = 0x0f;
    }
  }
  SPDR = c;
}

OWSerialClass::OWSerialClass() :
  _rx_buffer_head(0), _rx_buffer_tail(0),
  _tx_buffer_head(0), _tx_buffer_tail(0),
  _rx_escape(0), _tx_escape(0) {}

OWSerialClass OWSerial;

ISR(SPI_STC_vect) {
  OWSerial._interrupt();
}

void OWSerialClass::begin() {
  pinMode(SS, INPUT_PULLUP);
  pinMode(SCK, INPUT);
  pinMode(MISO, OUTPUT);
  pinMode(MOSI, INPUT);
  SPCR = _BV(SPE) | _BV(SPIE) | _BV(DORD);
}

void OWSerialClass::end() {
  SPCR = 0;
  pinMode(MISO, INPUT);
  pinMode(SS, INPUT);
  _rx_buffer_head = _rx_buffer_tail;
  _tx_buffer_head = _tx_buffer_tail;
  _rx_escape = _tx_escape = 0;
}

int OWSerialClass::available(void) {
  return OWS_RX_RING(OWS_RX_BUFFER_SIZE + _rx_buffer_head - _rx_buffer_tail);
}

int OWSerialClass::peek(void) {
  if (_rx_buffer_head == _rx_buffer_tail)
    return -1;
  return _rx_buffer[_rx_buffer_tail];
}

int OWSerialClass::read(void) {
  if (_rx_buffer_head == _rx_buffer_tail)
    return -1;
  uint8_t c = _rx_buffer[_rx_buffer_tail];
  _rx_buffer_tail = OWS_RX_RING(_rx_buffer_tail + 1);
  return c;
}

void OWSerialClass::flush(void) {}

size_t OWSerialClass::write(uint8_t c) {
  uint8_t i = OWS_TX_RING(_tx_buffer_head + 1);
  if (i == _tx_buffer_tail)
    return 0;
  _tx_buffer[_tx_buffer_head] = c;
  _tx_buffer_head = i;
  return 1;
}

