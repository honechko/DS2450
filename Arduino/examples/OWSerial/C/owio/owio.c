#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#define OWS_RX_BUFFER_SIZE 128
#define OWS_TX_BUFFER_SIZE 128

#if ((OWS_RX_BUFFER_SIZE) - 1) & (OWS_RX_BUFFER_SIZE)
#define OWS_RX_RING(i) ((i) % (OWS_RX_BUFFER_SIZE))
#else
#define OWS_RX_RING(i) ((i) & ((OWS_RX_BUFFER_SIZE) - 1))
#endif
#if ((OWS_TX_BUFFER_SIZE) - 1) & (OWS_TX_BUFFER_SIZE)
#define OWS_TX_RING(i) ((i) % (OWS_TX_BUFFER_SIZE))
#else
#define OWS_TX_RING(i) ((i) & ((OWS_TX_BUFFER_SIZE) - 1))
#endif

#ifndef SS_BIT
#define SS_BIT   2
#endif
#ifndef MOSI_BIT
#define MOSI_BIT 3
#endif
#ifndef MISO_BIT
#define MISO_BIT 4
#endif
#ifndef SCK_BIT
#define SCK_BIT  5
#endif
#ifndef SPI_PORT
#define SPI_PORT PORTB
#endif
#ifndef SPI_DDR
#define SPI_DDR  DDRB
#endif

static volatile uint8_t _rx_buffer_head;
static volatile uint8_t _rx_buffer_tail;
static volatile uint8_t _tx_buffer_head;
static volatile uint8_t _tx_buffer_tail;
static uint8_t _rx_escape;
static uint8_t _tx_escape;
static uint8_t _rx_buffer[OWS_RX_BUFFER_SIZE];
static uint8_t _tx_buffer[OWS_TX_BUFFER_SIZE];

ISR(SPI_STC_vect) {
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

static int ows_getc(FILE *stream) {
  uint8_t c;
  if (_rx_buffer_head == _rx_buffer_tail)
    return EOF;
  c = _rx_buffer[_rx_buffer_tail];
  _rx_buffer_tail = OWS_RX_RING(_rx_buffer_tail + 1);
  return c;
}
static int ows_putc(char c, FILE *stream) {
  uint8_t i;
  if (c == '\n') ows_putc('\r', stream);
  i = OWS_TX_RING(_tx_buffer_head + 1);
  if (i == _tx_buffer_tail)
    return -1;
  _tx_buffer[_tx_buffer_head] = c;
  _tx_buffer_head = i;
  return 0;
}
static FILE ows_input  = FDEV_SETUP_STREAM(NULL, ows_getc, _FDEV_SETUP_READ);
static FILE ows_output = FDEV_SETUP_STREAM(ows_putc, NULL, _FDEV_SETUP_WRITE);

void owio_start(void) {
  _rx_buffer_head = _rx_buffer_tail = 0;
  _tx_buffer_head = _tx_buffer_tail = 0;
  _rx_escape = _tx_escape = 0;
  SPI_PORT |= _BV(SS_BIT);
  SPI_DDR |= _BV(MISO_BIT);
  SPCR = _BV(SPE) | _BV(SPIE) | _BV(DORD);
  stdin  = &ows_input;
  stdout = &ows_output;
  sei();
}

void owio_stop(void) {
  SPI_DDR  &= ~(_BV(SS_BIT) | _BV(SCK_BIT) | _BV(MISO_BIT) | _BV(MOSI_BIT));
  SPI_PORT &= ~(_BV(SS_BIT) | _BV(SCK_BIT) | _BV(MISO_BIT) | _BV(MOSI_BIT));
  SPCR = 0;
  stdin = stdout = NULL;
}
