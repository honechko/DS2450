# DS2450 Library for Arduino

[DS2450.zip](https://github.com/honechko/DS2450/raw/main/Arduino/download/DS2450.zip)

## Dependencies

* [OneWire](https://www.pjrc.com/teensy/td_libs_OneWire.html), modified version with weak pullup [OneWire-2.3.7wp.zip](https://github.com/honechko/DS2450/raw/main/Arduino/download/OneWire-2.3.7wp.zip)
* [AltSoftSerial](https://www.pjrc.com/teensy/td_libs_AltSoftSerial.html), [AltSoftSerial.zip](https://github.com/honechko/DS2480/raw/main/Arduino/download/AltSoftSerial.zip)
* [DS2480B](https://github.com/collin80/DS2480B), fixed version [DS2480B_fixed.zip](https://github.com/honechko/DS2480/raw/main/Arduino/download/DS2480B_fixed.zip)

## Usage

```
#include <DS2450.h>
OneWire ow(10);
DS2450 pex(&ow);

byte addr[] = {0x20,0x41,0x42,0x0F,0x48,0x4E,0x59,0x49};
#define pin 0

void setup(void) {
  pex.pinMode(addr, pin, OUTPUT);
}

void loop(void) {
  if (pex.alarmPorIsSet(addr)) {
    pex.alarmPorClear(addr);
    pex.pinMode(addr, pin, OUTPUT);
  }
  pex.digitalWrite(addr, pin, HIGH);
  delay(500);
  pex.digitalWrite(addr, pin, LOW);
  delay(500);
}
```

Look examples subdir for more examples.

## Library functions

* ```uint8_t pinMode(uint8_t *addr, uint8_t pin, uint8_t mode)```  
  Sets pin mode for input or output  
  return value: true on success, false on error  
  pin: 0 | 1 | 2 | 3 | 4  
  mode: INPUT | INPUT_PULLUP | OUTPUT  
* ```uint8_t digitalWrite(uint8_t *addr, uint8_t pin, uint8_t val)```  
  Sets pin to high or low for output mode and to high impedance or pullup for input mode  
  return value: true on success, false on error  
  pin: 0 | 1 | 2 | 3 | 4  
  val: LOW | HIGH  
* ```uint8_t digitalRead(uint8_t *addr, uint8_t pin)```  
  Reads logical value from port  
  return value: LOW | HIGH  
  pin: 0 | 1 | 2 | 3 | 4  
* ```uint8_t analoglWrite(uint8_t *addr, uint8_t pin, uint8_t aval)```  
  Sets pin PWM mode and duty cycle  
  return value: true on success, false on error  
  pin: 1  
  aval: 0..255  
* ```uint8_t analogConvert(uint8_t *addr, uint8_t pin, ...)```  
  Runs ADC for specified port list, results are stored in device memory. addr = NULL corresponds to all devices on bus  
  return value: true on success, false on error  
  addr: addr | NULL  
  pin: 0 | 2 | 3 | 4  
* ```uint16_t analogReadLast(uint8_t *addr, uint8_t pin)```  
  Reads last ADC result from device memory  
  return value: 16-bit ADC result  
  pin: 0 | 2 | 3 | 4  
* ```uint16_t analogRead(uint8_t *addr, uint8_t pin)```  
  Runs ADC and reads results  
  return value: 16-bit ADC result  
  pin: 0 | 2 | 3 | 4  
* ```uint8_t analogResolution(uint8_t *addr, uint8_t pin, uint8_t res)```  
  Sets resolution in bits for next ADC  
  return value: true on success, false on error  
  pin: 0 | 2 | 3 | 4  
  res: 1..16  
* ```uint8_t analogReference(uint8_t *addr, uint8_t pin, uint8_t ref)```  
  Sets reference for next ADC, DEFAULT corresponds to VCC, INTERNAL corresponds to 1.1V  
  return value: true on success, false on error  
  pin: 0 | 2 | 3 | 4  
  ref: DEFAULT | INTERNAL  
* ```uint8_t alarmPorIsSet(uint8_t *addr)```  
  Reads Power-on reset (POR) flag  
  return value: flag value  
* ```uint8_t alarmPorClear(uint8_t *addr)```  
  Clears Power-on reset (POR) flag  
  return value: true on success, false on error  
* ```uint8_t alarmDigitalEnable(uint8_t *addr, uint8_t pin, uint8_t enable, uint8_t val)```  
  Sets logical level alarm mode  
  return value: true on success, false on error  
  pin: 0 | 1 | 2 | 3 | 4  
  enable: true | false  
  val: LOW | HIGH  
* ```uint8_t alarmDigitalIsSet(uint8_t *addr, uint8_t pin)```  
  Reads logical level alarm flag  
  return value: flag value  
  pin: 0 | 1 | 2 | 3 | 4  
* ```uint8_t alarmDigitalClear(uint8_t *addr, uint8_t pin)```  
  Clears logical level alarm flag  
  return value: true on success, false on error  
  pin: 0 | 1 | 2 | 3 | 4  
* ```uint8_t alarmAnalogHighEnable(uint8_t *addr, uint8_t pin, uint8_t enable, uint8_t threshold)```  
  Sets analog value too high alarm mode and threshold high value  
  return value: true on success, false on error  
  pin: 0 | 2 | 3 | 4  
  enable: true | false  
  threshold: 0..255  
* ```uint8_t alarmAnalogLowEnable(uint8_t *addr, uint8_t pin, uint8_t enable, uint8_t threshold)```  
  Sets analog value too low alarm mode and threshold low value  
  return value: true on success, false on error  
  pin: 0 | 2 | 3 | 4  
  enable: true | false  
  threshold: 0..255  
* ```uint8_t alarmAnalogHighIsSet(uint8_t *addr, uint8_t pin)```  
  Reads analog value too high alarm flag  
  return value: flag value  
  pin: 0 | 2 | 3 | 4  
* ```uint8_t alarmAnalogLowIsSet(uint8_t *addr, uint8_t pin)```  
  Reads analog value too low alarm flag  
  return value: flag value  
  pin: 0 | 2 | 3 | 4  
* ```uint8_t alarmAnalogClear(uint8_t *addr, uint8_t pin)```  
  Clears analog value too high and too low alarm flags  
  return value: true on success, false on error  
  pin: 0 | 2 | 3 | 4  
* ```uint8_t alarmClearAll(uint8_t *addr)```  
  Clears all alarm flags  
  return value: true on success, false on error  

