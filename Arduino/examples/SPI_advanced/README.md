# Example of SPI usage

1. Make connections  
   ![SPI over 1-wire](https://github.com/honechko/DS2450/raw/main/Arduino/examples/SPI_advanced/spi.jpg)

1. Fix address of your 1-wire slave device in source code and compile firmware  
   ```byte addr[] = {0x20,0x41,0x42,0x0F,0x48,0x4E,0x59,0x49};```  

1. Turn _local_ Arduino into "AVRISP over 1-wire"  
   ```avrdude -P /dev/ttyUSB0 -c arduino -p m328p -U flash:w:ArduinoOWISP.hex```  

1. Burn SPI-slave firmware to _remote_ Arduino  
   ```avrdude -P /dev/ttyUSB0 -b 9600 -c avrisp -p m328p -U flash:w:OWSPI_slave.hex```  
   Note: with *-c avrisp* avrdude does not reset _local_ Arduino and so communicates with ArduinoOWISP firmware,
   while with *-c arduino* avrdude does reset _local_ Arduino and so communicates with its bootloader.  

1. Burn SPI-master firmware to _local_ Arduino  
   ```avrdude -P /dev/ttyUSB0 -c arduino -p m328p -U flash:w:OWSPI_master.hex```  

1. View output from _local_ Arduino  
   ```minicom -D /dev/ttyUSB0 -b 9600```  

