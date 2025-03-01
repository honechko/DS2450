# Serial over SPI over 1-wire

This input-output C library can be used in place of standard stdio library.
It allows bidirectional serial communication with uC over 1-wire and
requires minimal modifications of your code.  

## How to use

1. Make connections:  
   ![SPI over 1-wire](https://github.com/honechko/DS2450/raw/main/Arduino/examples/SPI_advanced/spi.jpg)  
   Left Arduino is named _local_, connect it to host with USB-cable.
   Right Arduino is named _remote_, owio library is intended for it.
   You can connect multiple 1-wire slave devices to single bus.

1. Burn [ArduinoOWISP](https://github.com/honechko/DS2450/tree/main/Arduino/examples/ArduinoOWISP)
   firmware to _local_ Arduino:  
   ```avrdude -P /dev/ttyUSB0 -c arduino -p m328p -U flash:w:ArduinoOWISP.hex```

1. Burn firmware that uses owio library to _remote_ Arduino:  
   ```avrdude -P /dev/ttyUSB0 -b 19200 -c avrisp -p m328p -U flash:w:main.hex```  
   Note: with ```-c avrisp``` avrdude communicates with ArduinoOWISP firmware
   on _local_ Arduino and affects _remote_ Arduino, while with ```-c arduino```
   avrdude communicates with bootloader on _local_ Arduino and affects
   _local_ Arduino.

1. View output from _remote_ Arduino (_local_ Arduino acts as a relay):  
   ```minicom -D /dev/ttyUSB0 -b 19200```  
   In opened minicom press Enter or Ctrl+M to activate serial connection.

