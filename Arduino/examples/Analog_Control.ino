#define HAVE_DS2480B 0
#if HAVE_DS2480B
#include <DS2480B.h>
AltSoftSerial altSerial; // RX on pin 8, TX on pin 9
DS2480B ow(altSerial);
#else
#include <OneWire.h>
OneWire ow(7); // DQ on pin 7, You also need 1k resistor between DQ and VCC
#endif

#include <DS2450.h>
DS2450 pex(&ow);

#define ANALOG_HIGH 200
#define ANALOG_LOW  50

void setup(void) {
  Serial.begin(9600);
}

void loop(void) {
  byte addr[8], i, n;
  while (ow.search(addr, false)) {
    if (addr[0] == 0x20) {
      Serial.print("ID:");
      for (i = 0; i < 8; i++) {
        Serial.write(' ');
        Serial.print(addr[i], HEX);
      }
      Serial.println();
      if (pex.alarmPorIsSet(addr)) {
        Serial.println(" Power-on Reset");
        for (n = i = 0; i < 5; i++) if (i != 1) {
          if (!pex.alarmAnalogHighEnable(addr, i, true, ANALOG_HIGH) ||
              !pex.alarmAnalogLowEnable( addr, i, true, ANALOG_LOW)) {
            Serial.print("  Failed setup pin ");
            Serial.println(i);
            n++;
          }
        }
        if (!n)
          pex.alarmPorClear(addr);
      } else {
        Serial.println(" Alarm other");
        pex.alarmClearAll(addr);
      }
    }
  }
  ow.reset_search();
  if (!pex.analogConvert(NULL, 0, 2, 3, 4)) {
    Serial.println("Failed analogConvert");
  } else {
    while (ow.search(addr, false)) {
      if (addr[0] == 0x20) {
        Serial.print("ID:");
        for (i = 0; i < 8; i++) {
          Serial.write(' ');
          Serial.print(addr[i], HEX);
        }
        Serial.println();
        if (!pex.alarmPorIsSet(addr)) {
          for (i = 0; i < 5; i++) if (i != 1) {
            if (pex.alarmAnalogHighIsSet(addr, i) ||
                pex.alarmAnalogLowIsSet( addr, i)) {
              Serial.print(" Alarm on pin ");
              Serial.print(i);
              Serial.print(" value = ");
              Serial.println((double)pex.analogReadLast(addr, i)/256);
              pex.alarmAnalogClear(addr, i);
            }
          }
        }
      }
    }
    ow.reset_search();
  }
  delay(1000);
}

