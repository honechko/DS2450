MCU     = attiny88
F_CPU   = 16000000
CC      = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude
PORT    = /dev/ttyUSB0
OWIODIR = ../..
CFLAGS  = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -I$(OWIODIR) -Os -mrelax

main.hex: main.elf
	$(OBJCOPY) -R .eeprom -O ihex main.elf main.hex
	avr-size main.elf

main.elf: main.o owio.o
	$(CC) $(CFLAGS) -o main.elf main.o owio.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

owio.o: $(OWIODIR)/owio.c $(OWIODIR)/owio.h
	$(CC) $(CFLAGS) -c $(OWIODIR)/owio.c

clean:
	rm -f *.o *.map *.elf main.hex

upload: main.hex
	$(AVRDUDE) -P $(PORT) -b 19200 -c avrisp -p $(MCU) -U flash:w:main.hex
	#minicom -D $(PORT) -b 19200

