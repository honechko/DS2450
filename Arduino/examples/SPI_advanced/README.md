# Arduino as 1-wire sensor/actuator

This example shows implementation of Remote Call based protocol over 1-wire
and built on top of it software sensor/actuator (OWRC_slave) and server-side
code (OWRC_master) to read values and send commands to this sensor/actuator.

## How to use

1. Make connections:  
   ![SPI over 1-wire](https://github.com/honechko/DS2450/raw/main/Arduino/examples/SPI_advanced/spi.jpg)  
   Left Arduino is named _local_, connect it to host with USB-cable.
   Right Arduino is named _remote_, software sensor/actuator code runs on it.
   You can connect multiple 1-wire slave devices to single bus.

1. Compile and burn OWRC_slave.hex firmware to _remote_ Arduino.  
   Note: You can do this remotely over 1-wire, see [ArduinoOWISP](https://github.com/honechko/DS2450/tree/main/Arduino/examples/ArduinoOWISP).

1. Find and fix address of your 1-wire slave device in OWRC_master.ino source code:  
   ```byte addr[] = {0x20,0x41,0x42,0x0F,0x48,0x4E,0x59,0x49};```  
   compile and burn OWRC_master.hex firmware to _local_ Arduino:  
   ```avrdude -P /dev/ttyUSB0 -c arduino -p m328p -U flash:w:OWRC_master.hex```

1. View serial output from _local_ Arduino:  
   ```minicom -D /dev/ttyUSB0 -b 9600```

