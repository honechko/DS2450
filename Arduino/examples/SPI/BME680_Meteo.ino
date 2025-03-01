#include <DS2450.h>
OneWire ow(7); // DQ on pin 7, You also need 1k resistor between DQ and VCC
DS2450 pex(&ow);

// DS2450   BME680
//  GND <--> GND
//  VCC <--> VCC
//  P0  <--> CS
//  P1
//  P2  <--> SCL
//  P3  <--> SDA
//  P4  <--> SDO

byte addr[] = {0x20,0x45,0x42,0x0F,0x48,0x4E,0x59,0x95};
#define CS 0

int8_t bme680(uint8_t *addr, uint8_t cs, int16_t *t, uint16_t *h, uint32_t *p) {
  uint8_t  buf[24], cnt = 0;
  uint16_t par_t1, par_h1, par_h2, par_p1;
  int16_t  par_t2, par_p2, par_p4, par_p5, par_p8, par_p9, par_g2;
  uint8_t  par_h6, par_p10;
  int8_t   par_t3, par_h3, par_h4, par_h5, par_h7, par_p3, par_p6, par_p7, par_g1, par_g3;
  int32_t  adc, t_fine, var1, var2;

  // start requested measurements
  pex.pinMode(addr, cs, OUTPUT);
  pex.spi_start(NULL);
  pex.spi_transfer_msbfirst(0x73);
  pex.spi_transfer_msbfirst(0x10);
  pex.spi_transfer_msbfirst(0x72);
  pex.spi_transfer_msbfirst(h ? 0x05 : 0x00);
  pex.spi_transfer_msbfirst(0x74);
  pex.spi_transfer_msbfirst(p ? 0xb5 : 0xa1);
  pex.pinMode(NULL, cs, INPUT);

  // read conversion parameters
  pex.pinMode(NULL, cs, OUTPUT);
  pex.spi_start(NULL);
  pex.spi_transfer_msbfirst(0x73);
  pex.spi_transfer_msbfirst(0x00);
  pex.spi_transfer_msbfirst(0x8a);
  pex.spi_transfer_msbfirst(buf, buf, 23);
  pex.pinMode(NULL, cs, INPUT);
  par_t2 = ((uint16_t)buf[1]  << 8) | buf[0];
  par_t3 = buf[2];
  par_p1 = ((uint16_t)buf[5] << 8) | buf[4];
  par_p2 = ((uint16_t)buf[7] << 8) | buf[6];
  par_p3 = buf[8];
  par_p4 = ((uint16_t)buf[11] << 8) | buf[10];
  par_p5 = ((uint16_t)buf[13] << 8) | buf[12];
  par_p7 = buf[14];
  par_p6 = buf[15];
  par_p8 = ((uint16_t)buf[19] << 8) | buf[18];
  par_p9 = ((uint16_t)buf[21] << 8) | buf[20];
  par_p10 = buf[22];
  pex.pinMode(NULL, cs, OUTPUT);
  pex.spi_start(NULL);
  pex.spi_transfer_msbfirst(0x73);
  pex.spi_transfer_msbfirst(0x00);
  pex.spi_transfer_msbfirst(0xe1);
  pex.spi_transfer_msbfirst(buf, buf, 14);
  pex.pinMode(NULL, cs, INPUT);
  par_h2 = ((uint16_t)buf[0] << 4) | (buf[1] >> 4);
  par_h1 = ((uint16_t)buf[2] << 4) | (buf[1] & 0x0f);
  par_h3 = buf[3];
  par_h4 = buf[4];
  par_h5 = buf[5];
  par_h6 = buf[6];
  par_h7 = buf[7];
  par_t1 = ((uint16_t)buf[9] << 8) | buf[8];
  par_g2 = ((uint16_t)buf[11] << 8) | buf[10];
  par_g1 = buf[12];
  par_g3 = buf[13];

  // wait for measurements complete
  do {
    if (++cnt >= 100)
      return -1;
    delay(50);
    pex.pinMode(NULL, cs, OUTPUT);
    pex.spi_start(NULL);
    pex.spi_transfer_msbfirst(0x73);
    pex.spi_transfer_msbfirst(0x10);
    pex.spi_transfer_msbfirst(0x9d);
    pex.spi_transfer_msbfirst(buf, buf, 1);
    pex.pinMode(NULL, cs, INPUT);
  } while (buf[0] & (1 << 5));

  // retrieve adc results
  pex.pinMode(NULL, cs, OUTPUT);
  pex.spi_start(NULL);
  pex.spi_transfer_msbfirst(0x73);
  pex.spi_transfer_msbfirst(0x10);
  pex.spi_transfer_msbfirst(0x9f);
  pex.spi_transfer_msbfirst(buf, buf, 8);
  pex.pinMode(NULL, cs, INPUT);

  // convert
  adc = ((uint32_t)buf[3] << 16) | ((uint32_t)buf[4] << 8) | (uint32_t)buf[5];
  var1 = adc - ((int32_t)par_t1 << 8);
  var2 = ((var1 >> 7) * (int32_t)par_t2) >> 11;
  var1 = ((var1 >> 8) * (var1 >> 8)) >> 8;
  var1 = (var1 * (int32_t)par_t3) >> 14;
  t_fine = var1 + var2;
  if (t)
    *t = (t_fine * 5 + 128) >> 8;

  if (p) {
    adc = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | (uint32_t)buf[2];
    var1 = t_fine - 128000;
    var2 = (((((var1 >> 3) * (var1 >> 3)) >> 11) * (int32_t)par_p6) >> 2) + var1 * (int32_t)par_p5;
    var2 = (var2 >> 2) + ((int32_t)par_p4 << 16);
    var1 = (((((var1 >> 3) * (var1 >> 3)) >> 13) * ((int32_t)par_p3 << 5)) >> 3) + (((int32_t)par_p2 * var1) >> 2);
    var1 = (((var1 >> 18) + 32768) * (int32_t)par_p1) >> 15;
    var2 = ((1048576 - (adc >> 4)) - (var2 >> 12)) * ((uint32_t)3125);
    if (var2 >= 0x40000000)
      var2 = ((var2 / var1) << 1);
    else
      var2 = ((var2 << 1) / var1);
    var1 = ((int32_t)par_p7 << 7) + (((var2 >> 2) * (int32_t)par_p8) >> 13) +
           (((((var2 >> 3) * (var2 >> 3)) >> 13) * (int32_t)par_p9) >> 12) +
           (((var2 >> 8) * (var2 >> 8) * (var2 >> 8) * (int32_t)par_p10) >> 17);
    *p = var2 + (var1 >> 4);
  }

  if (h) {
    adc = ((uint16_t)buf[6] << 8) | (uint16_t)buf[7];
    t_fine = (t_fine + 10) / 20;
    var1 = adc - ((int32_t)par_h1 << 4) - ((t_fine * (int32_t)par_h3) >> 9);
    var2 = ((int32_t)par_h2 * (((int32_t)1 << 14) + ((t_fine * (int32_t)par_h4) >> 8) +
            ((t_fine * ((t_fine * (int32_t)par_h5) >> 8)) >> 14))) >> 10;
    var1 = var1 * var2;
    var2 = (((int32_t)par_h6 << 7) + ((t_fine * (int32_t)par_h7) >> 8)) >> 4;
    var2 = (var2 * (((var1 >> 14) * (var1 >> 14)) >> 10)) >> 1;
    var1 = (((var1 + var2) >> 10) * 100 + 2048) >> 12;
    if (var1 > 10000)
      var1 = 10000;
    else if (var1 < 0)
      var1 = 0;
    *h = var1;
  }

  return 0;
}

void setup(void) {
  Serial.begin(9600);
}

void loop(void) {
  int16_t  t;
  uint16_t h;
  uint32_t p;

  if (!bme680(addr, CS, &t, &h, &p)) {
    Serial.print("t = ");
    Serial.println(t / 100.0);
    Serial.print("h = ");
    Serial.println(h / 100.0);
    Serial.print("p = ");
    Serial.println(p);
  }
  delay(1000);
}

