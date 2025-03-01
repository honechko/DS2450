#ifndef OWSerial_h
#define OWSerial_h

#include <inttypes.h>
#include <Stream.h>

#ifndef OWS_RX_BUFFER_SIZE
#define OWS_RX_BUFFER_SIZE 128
#endif
#ifndef OWS_TX_BUFFER_SIZE
#define OWS_TX_BUFFER_SIZE 128
#endif

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

class OWSerialClass : public Stream {
protected:
  volatile uint8_t _rx_buffer_head;
  volatile uint8_t _rx_buffer_tail;
  volatile uint8_t _tx_buffer_head;
  volatile uint8_t _tx_buffer_tail;
  uint8_t _rx_escape;
  uint8_t _tx_escape;
  uint8_t _rx_buffer[OWS_RX_BUFFER_SIZE];
  uint8_t _tx_buffer[OWS_TX_BUFFER_SIZE];
public:
  inline OWSerialClass();
  void begin();
  void begin(unsigned long) { begin(); }
  void end();
  virtual int available(void);
  virtual int peek(void);
  virtual int read(void);
  virtual void flush(void);
  virtual size_t write(uint8_t byte);
  using Print::write;
  operator bool() { return true; }
  inline void _interrupt(void);
};

extern OWSerialClass OWSerial;

#endif
