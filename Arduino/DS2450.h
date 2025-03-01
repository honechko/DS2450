#ifndef DS2450_h
#define DS2450_h

#ifndef HAVE_DS2480B
#define HAVE_DS2480B 0
#endif

#ifndef INTERNAL
#define INTERNAL INTERNAL1V1
#endif

#if HAVE_DS2480B
#include <DS2480B.h>
#else
#include <OneWire.h>
#endif

const uint8_t PROGMEM DS2450_pin_to_addr0_PGM[] = {0x08,0x1f,0x0a,0x0c,0x0e};
const uint8_t PROGMEM DS2450_pin_to_addr1_PGM[] = {0x09,0x1f,0x0b,0x0d,0x0f};
#define to_end_of_page(base) ((7 & ~(base)) + 1)

class DS2450 {
  private:
#if HAVE_DS2480B
    DS2480B *_ow;
#else
    OneWire *_ow;
#endif
    uint8_t _mask(uint8_t pin) {
      return pin != 4 ? pin != 3 ? pin != 2 ? pin ? 0 : 1 : 2 : 4 : 8;
    };
    uint8_t _breverse(uint8_t v) {
      uint8_t t;
      t = v = (v << 1) + (v >> 7);    // bcdefgha
      v = ((v << 1) + (v >> 7)) << 1; // defghab0
      t = v = ((v ^ t) & 0xaa) ^ t;   // dcfehgba
      v = (v << 4) | (v >> 4);        // hgbadcfe
      return  ((v ^ t) & 0xcc) ^ t;   // hgfedcba
    }

  public:
#if HAVE_DS2480B
    DS2450(DS2480B *ow) {
#else
    DS2450(OneWire *ow) {
#endif
      _ow = ow;
    };

    uint8_t read(uint8_t *addr, uint8_t *buf, uint8_t start, uint8_t cnt) {
      uint8_t i, tbuf[3];
      uint16_t crc;
      _ow->reset();
      if (addr)
        _ow->select(addr);
      else
        _ow->write(0xa5); // RESUME
      tbuf[0] = 0xaa; // READ MEMORY
      tbuf[1] = start;
      tbuf[2] = 0;
      _ow->write_bytes(tbuf, 3);
      crc = _ow->crc16(tbuf, 3);
      for (i = 0; i < cnt; i++) {
        buf[i] = _ow->read();
        crc = _ow->crc16(&buf[i], 1, crc);
        if (!(++start & 0x07)) {
          _ow->read_bytes(tbuf, 2);
          if (!_ow->check_crc16(NULL, 0, tbuf, crc))
            return i >= 7 ? i - 7 : 0;
          crc = 0;
        }
      }
      return i;
    };
    uint8_t write(uint8_t *addr, uint8_t *buf, uint8_t start, uint8_t cnt) {
      uint8_t i, tbuf[3];
      uint16_t crc;
      _ow->reset();
      if (addr)
        _ow->select(addr);
      else
        _ow->write(0xa5); // RESUME
      tbuf[0] = 0x55; // WRITE MEMORY
      tbuf[1] = start;
      tbuf[2] = 0;
      _ow->write_bytes(tbuf, 3);
      crc = _ow->crc16(tbuf, 3);
      for (i = 0; i < cnt; i++) {
        _ow->write(buf[i]);
        crc = ~_ow->crc16(&buf[i], 1, crc);
        if (_ow->read() != (crc & 0xff) || _ow->read() != (crc >> 8) ||
            _ow->read() != buf[i])
          break;
        crc = ++start;
      }
      return i;
    };
    void spi_start(uint8_t *addr) {
      _ow->reset();
      if (addr)
        _ow->select(addr);
      else
        _ow->write(0xa5); // RESUME
      _ow->write(0x5a); // SPI
    }
    uint8_t spi_transfer_lsbfirst(uint8_t v) {
      _ow->write(v);
      return _ow->read();
    }
    uint8_t spi_transfer_msbfirst(uint8_t v) {
      _ow->write(_breverse(v));
      return _breverse(_ow->read());
    }
    uint16_t spi_transfer16_lsbfirst(uint16_t v) {
      uint8_t r;
      _ow->write(v & 0xff);
      r = _ow->read();
      _ow->write(v >> 8);
      return (_ow->read() << 8) | r;
    }
    uint16_t spi_transfer16_msbfirst(uint16_t v) {
      uint8_t r;
      _ow->write(_breverse(v >> 8));
      r = _breverse(_ow->read());
      _ow->write(_breverse(v & 0xff));
      return _breverse(_ow->read()) | (r << 8);
    }
    void spi_transfer_lsbfirst(const uint8_t *sbuf, uint8_t *rbuf, size_t cnt) {
      while (cnt--) {
        _ow->write(*sbuf++);
        *rbuf++ = _ow->read();
      }
    }
    void spi_transfer_msbfirst(const uint8_t *sbuf, uint8_t *rbuf, size_t cnt) {
      while (cnt--) {
        _ow->write(_breverse(*sbuf++));
        *rbuf++ = _breverse(_ow->read());
      }
    }
    void spi_transfer_lsbfirst_P(const uint8_t *sbuf, uint8_t *rbuf, size_t cnt) {
      while (cnt--) {
        _ow->write(pgm_read_byte(sbuf++));
        *rbuf++ = _ow->read();
      }
    }
    void spi_transfer_msbfirst_P(const uint8_t *sbuf, uint8_t *rbuf, size_t cnt) {
      while (cnt--) {
        _ow->write(_breverse(pgm_read_byte(sbuf++)));
        *rbuf++ = _breverse(_ow->read());
      }
    }

    uint8_t analogConvert(uint8_t *addr, uint8_t pin,
                          uint8_t pin2 = 5, uint8_t pin3 = 5, uint8_t pin4 = 5) {
      uint8_t m, tbuf[3];
      uint16_t crc;
      m  = _mask(pin) | _mask(pin2) | _mask(pin3) | _mask(pin4);
      if (!m)
        return 0;
      _ow->reset();
      if (addr)
        _ow->select(addr);
      else
        _ow->skip();
      tbuf[0] = 0x3c; // CONVERT V
      tbuf[1] = m;
      tbuf[2] = 0;
      _ow->write_bytes(tbuf, 3);
      crc = _ow->crc16(tbuf, 3);
      _ow->read_bytes(tbuf, 2);
      if (!_ow->check_crc16(NULL, 0, tbuf, crc))
        return 0;
      delay(6);
      while (!_ow->read_bit());
      return 1;
    };

    uint8_t pinMode(uint8_t *addr, uint8_t pin, uint8_t mode) {
      uint8_t base, buf[8], r;
      if (pin >= 5)
        return 0;
      base = pgm_read_byte(DS2450_pin_to_addr0_PGM + pin);
      if (!read(addr, buf, base, to_end_of_page(base)))
        return 0;
      r = buf[0];
      if (mode == INPUT)
        r &= ~0xc0;
      else if (mode == INPUT_PULLUP)
        r = (r & ~0xc0) | 0x40;
      else
        r |= 0x80;
      if (pin == 1) {
        r &= ~0x04;
        if ((r & 0xc0) == 0xc0) {
          buf[0] = 0xff;
          buf[1] = r;
          return (write(NULL, buf, 0x1e, 2) == 2);
        }
      }
      buf[0] = r;
      return (write(NULL, buf, base, 1) == 1);
    };
    uint8_t pinMode(uint8_t *addr, uint8_t pin, uint8_t mode, uint8_t val) {
      uint8_t base, buf[8], r;
      if (pin >= 5)
        return 0;
      base = pgm_read_byte(DS2450_pin_to_addr0_PGM + pin);
      if (!read(addr, buf, base, to_end_of_page(base)))
        return 0;
      r = buf[0] & ~0xc0;
      if (mode == OUTPUT)
        r |= 0x80;
      if (val != LOW)
        r |= 0x40;
      if (pin == 1) {
        r &= ~0x04;
        if ((r & 0xc0) == 0xc0) {
          buf[0] = 0xff;
          buf[1] = r;
          return (write(NULL, buf, 0x1e, 2) == 2);
        }
      }
      buf[0] = r;
      return (write(NULL, buf, base, 1) == 1);
    };
    uint8_t digitalWrite(uint8_t *addr, uint8_t pin, uint8_t val) {
      uint8_t base, buf[8], r;
      if (pin >= 5)
        return 0;
      base = pgm_read_byte(DS2450_pin_to_addr0_PGM + pin);
      if (!read(addr, buf, base, to_end_of_page(base)))
        return 0;
      r = buf[0] & ~0x40;
      if (val != LOW)
        r |= 0x40;
      if (pin == 1) {
        r &= ~0x04;
        if ((r & 0xc0) == 0xc0) {
          buf[1] = r;
          buf[0] = 0xff;
          return (write(NULL, buf, 0x1e, 2) == 2);
        }
      }
      buf[0] = r;
      return (write(NULL, buf, base, 1) == 1);
    };
    uint8_t digitalRead(uint8_t *addr, uint8_t pin) {
      uint8_t base, buf[8];
      if (pin >= 5)
        return LOW;
      base = pgm_read_byte(DS2450_pin_to_addr0_PGM + pin);
      return (read(addr, buf, base, to_end_of_page(base)) && (*buf & 0x20)) ?
             HIGH : LOW;
    };
    uint8_t analogWrite(uint8_t *addr, uint8_t pin, uint8_t val) {
      uint8_t buf[2];
      if (pin != 1 || !read(addr, buf + 1, 0x1f, 1))
        return 0;
      buf[1] = (buf[0] & ~0x04) | 0xc0;
      buf[0] = val;
      return (write(NULL, buf, 0x1e, 2) == 2);
    };

    uint16_t analogReadLast(uint8_t *addr, uint8_t pin) {
      uint8_t base, buf[8];
      if (pin >= 5 || pin == 1)
        return 0;
      base = pgm_read_byte(DS2450_pin_to_addr0_PGM + pin);
      if (!read(addr, buf, base - 0x08, 0x10 - base))
        return 0;
      return makeWord(buf[1], buf[0]);
    };
    uint16_t analogRead(uint8_t *addr, uint8_t pin) {
      return analogConvert(addr, pin) ? analogReadLast(addr, pin) : 0;
    };
    uint8_t analogResolution(uint8_t *addr, uint8_t pin, uint8_t val) {
      uint8_t base, buf[8];
      if (pin >= 5 || pin == 1)
        return 0;
      base = pgm_read_byte(DS2450_pin_to_addr0_PGM + pin);
      if (!read(addr, buf, base, 0x10 - base))
        return 0;
      buf[0] = (buf[0] & 0xf0) | (val & 0x0f);
      return (write(NULL, buf, base, 1) == 1);
    };
    uint8_t analogReference(uint8_t *addr, uint8_t pin, uint8_t val) {
      uint8_t base, buf[8];
      if (pin >= 5 || pin == 1)
        return 0;
      base = pgm_read_byte(DS2450_pin_to_addr0_PGM + pin);
      if (!read(addr, buf, base, 0x10 - base))
        return 0;
      if (val == DEFAULT)
        buf[0] &= ~0x10;
      else if (val == INTERNAL)
        buf[0] |=  0x10;
      else
        return 0;
      return (write(NULL, buf, base, 1) == 1);
    };

    uint8_t alarmPorIsSet(uint8_t *addr) {
      uint8_t buf[1];
      return (!read(addr, buf, 0x0f, 1) || (buf[0] & 0x80));
    }
    uint8_t alarmPorClear(uint8_t *addr) {
      uint8_t buf[1];
      if (!read(addr, buf, 0x0f, 1))
        return 0;
      buf[0] &= ~0xc0;
      return (write(NULL, buf, 0x0f, 1) == 1);
    }

    uint8_t alarmDigitalEnable(uint8_t *addr, uint8_t pin, uint8_t enable,
                               uint8_t val = LOW) {
      uint8_t base, buf[7], r;
      if (pin >= 5)
        return 0;
      base = pgm_read_byte(DS2450_pin_to_addr1_PGM + pin);
      if (!read(addr, buf, base, to_end_of_page(base)))
        return 0;
      r = buf[0] & (pin == 1 ? ~0x07 : ~0x43);
      if (enable)
        r |= 0x02;
      if (val != LOW)
        r |= 0x01;
      buf[0] = r;
      return (write(NULL, buf, base, 1) == 1);
    }
    uint8_t alarmDigitalIsSet(uint8_t *addr, uint8_t pin) {
      uint8_t base, buf[7];
      if (pin >= 5)
        return 0;
      base = pgm_read_byte(DS2450_pin_to_addr1_PGM + pin);
      return (!read(addr, buf, base, to_end_of_page(base)) ||
              (buf[0] & (pin == 1 ? 0x04 : 0x40)));
    }
    uint8_t alarmDigitalClear(uint8_t *addr, uint8_t pin) {
      uint8_t base, buf[7];
      if (pin >= 5)
        return 0;
      base = pgm_read_byte(DS2450_pin_to_addr1_PGM + pin);
      if (!read(addr, buf, base, to_end_of_page(base)))
        return 0;
      buf[0] |= (pin == 1 ? 0x04 : 0x40);
      return (write(NULL, buf, base, 1) == 1);
    }

    uint8_t alarmAnalogHighEnable(uint8_t *addr, uint8_t pin, uint8_t enable,
                                  uint8_t val = 255) {
      uint8_t base, buf[7], r;
      if (pin >= 5 || pin == 1)
        return 0;
      base = pgm_read_byte(DS2450_pin_to_addr1_PGM + pin);
      if (!read(addr, buf, base, 0x10 - base))
        return 0;
      r = buf[0] & ~0x60;
      if (enable)
        r |= 0x20;
      buf[0] = r;
      buf[6] = val;
      return (write(NULL, buf + 6, base + 8, 1) == 1 &&
              write(NULL, buf, base, 1) == 1);
    }
    uint8_t alarmAnalogLowEnable(uint8_t *addr, uint8_t pin, uint8_t enable,
                                 uint8_t val = 0) {
      uint8_t base, buf[7], r;
      if (pin >= 5 || pin == 1)
        return 0;
      base = pgm_read_byte(DS2450_pin_to_addr1_PGM + pin);
      if (!read(addr, buf, base, 0x10 - base))
        return 0;
      r = buf[0] & ~0x50;
      if (enable)
        r |= 0x10;
      buf[0] = r;
      buf[6] = val;
      return (write(NULL, buf + 6, base + 7, 1) == 1 &&
              write(NULL, buf, base, 1) == 1);
    }
    uint8_t alarmAnalogHighIsSet(uint8_t *addr, uint8_t pin) {
      uint8_t base, buf[7];
      if (pin >= 5 || pin == 1)
        return 0;
      base = pgm_read_byte(DS2450_pin_to_addr1_PGM + pin);
      return (!read(addr, buf, base, 0x10 - base) || (buf[0] & 0x20));
    }
    uint8_t alarmAnalogLowIsSet(uint8_t *addr, uint8_t pin) {
      uint8_t base, buf[7];
      if (pin >= 5 || pin == 1)
        return 0;
      base = pgm_read_byte(DS2450_pin_to_addr1_PGM + pin);
      return (!read(addr, buf, base, 0x10 - base) || (buf[0] & 0x10));
    }
    uint8_t alarmAnalogClear(uint8_t *addr, uint8_t pin) {
      uint8_t base, buf[7];
      if (pin >= 5 || pin == 1)
        return 0;
      base = pgm_read_byte(DS2450_pin_to_addr1_PGM + pin);
      if (!read(addr, buf, base, 0x10 - base))
        return 0;
      buf[0] &= ~0x70;
      return (write(NULL, buf, base, 1) == 1);
    }

    uint8_t alarmClearAll(uint8_t *addr) {
      uint8_t buf[8], i;
      if (!read(addr, buf, 0x09, 7) || !read(addr, buf + 7, 0x1f, 1))
        return 0;
      for (i = 0; i < 8; i += 2)
        buf[i] = (buf[i] & ~0xf0) | 0x40;
      buf[7] |= 0x04;
      return (write(NULL, buf + 7, 0x1f, 1) == 1 &&
              write(NULL, buf, 0x09, 7) == 7);
    }
};

#endif
