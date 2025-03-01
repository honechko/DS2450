# Serial over SPI over 1-wire

This Arduino library can be used in place of standard Serial library.
It allows bidirectional serial communication with uC over 1-wire and
requires minimal modifications of your code.  

## How to use

1. Make connections:  
   ![SPI over 1-wire](https://github.com/honechko/DS2450/raw/main/Arduino/examples/SPI_advanced/spi.jpg)  
   Left Arduino is named _local_, connect it to host with USB-cable.
   Right Arduino is named _remote_, OWSerial library is intended for it.
   You can connect multiple 1-wire slave devices to single bus.

1. Burn firmware that uses OWSerial library to _remote_ Arduino.  
   Note: You can do this remotely over 1-wire, see [ArduinoOWISP](https://github.com/honechko/DS2450/tree/main/Arduino/examples/ArduinoOWISP).

1. Find and fix address of your 1-wire slave device in OWSerialMaster.ino
   source code:  
   ```byte addr[] = {0x20,0x41,0x42,0x0F,0x48,0x4E,0x59,0x49};```  
   compile and burn OWSerialMaster.hex firmware to _local_ Arduino.  
   Note: [ArduinoOWISP](https://github.com/honechko/DS2450/tree/main/Arduino/examples/ArduinoOWISP)
   has built-in functionality of OWSerialMaster, which can be activated by
   sending CR (Enter or Ctrl+M) in minicom at next step.

1. View output from _remote_ Arduino (_local_ Arduino acts as a relay):  
   ```minicom -D /dev/ttyUSB0 -b 19200```

## Code modification

* Example with standard Serial library:  
```
void setup(void) {
  Serial.begin(9600);
}

void loop(void) {
  Serial.println("Hello, World!");
  delay(1000);
}
```

* The same example with OWSerial library:  
```
#include <OWSerial.h>

void setup(void) {
  OWSerial.begin();
}

void loop(void) {
  OWSerial.println("Hello, World!");
  delay(1000);
}
```

* And the same example with only two added lines:  
```
#include <OWSerial.h>
#define Serial OWSerial

void setup(void) {
  Serial.begin(9600);
}

void loop(void) {
  Serial.println("Hello, World!");
  delay(1000);
}
```

