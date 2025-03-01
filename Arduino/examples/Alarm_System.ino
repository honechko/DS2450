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
        for (n = i = 0; i < 5; i++) {
          if (!pex.pinMode(addr, i, INPUT_PULLUP) ||
              !pex.alarmDigitalEnable(addr, i, true, LOW)) {
            Serial.print("  Failed setup pin ");
            Serial.println(i);
            n++;
          }
        }
        if (!n)
          pex.alarmPorClear(addr);
      } else {
        for (n = i = 0; i < 5; i++) {
          if (pex.alarmDigitalIsSet(addr, i)) {
            Serial.print(" Alarm on pin ");
            Serial.println(i);
            pex.alarmDigitalClear(addr, i);
            n++;
          }
        }
        if (!n) {
          Serial.println(" Alarm other");
          pex.alarmClearAll(addr);
        }
      }
    }
  }
  ow.reset_search();
  delay(1000);
}

