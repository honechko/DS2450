1-wire port expander
====================

Device memory
-------------

**Memory pages**

Address space of the device consists of 4 memory pages, each 8 bytes in size:

| Page | Addresses | Contents |
| --- | --- | --- |
| 0   | 0x00 .. 0x07 | ADC results |
| 1   | 0x08 .. 0x0f | Control/status data |
| 2   | 0x10 .. 0x17 | Thresholds |
| 3   | 0x18 .. 0x1f | User data, control/status data |

The entire address space is available for reading and writing. Reading and writing data from/to memory addresses is byte-by-byte (commands 0xAA READ MEM, 0x55 WRITE MEM). ADC results are 16-bit values that are written into two memory cells by the device itself asynchronously after ADC conversion is completed sequentially for each channel (the conversion is initiated by 0x3c CONVERT command), the low and high bytes of ADC result are read from device atomically (the situation of reading low byte of old result and high byte of new result is excluded). When writing bytes that change port settings, the changes occur immediately after each byte is written.

**Alarm flags**

Device has 14 alarm flags: POR, AFD\[0..4\], AFL\[0,2..4\], AFH\[0,2..4\], if at least one of which is set, the device will respond to alarm search request. The device itself can change state of the flags (see description of the corresponding flags). All flags are also available for reading and changing by referring to bits at specific addresses. To exclude a device from alarm list, all of the alarm flags must be cleared.

**Addresses 0x00..0x07**

<table>
<tr><td>Bit</td><td>7</td><td>6</td><td>5</td><td>4</td><td>3</td><td>2</td><td>1</td><td>0</td></tr>
<tr><td>Address 0x00</td><td colspan=8 align=center>ADCV0:[7..0]</td></tr>
<tr><td>Address 0x01</td><td colspan=8 align=center>ADCV0:[15..8]</td></tr>
<tr><td>Address 0x02</td><td colspan=8 align=center>ADCV2:[7..0]</td></tr>
<tr><td>Address 0x03</td><td colspan=8 align=center>ADCV2:[15..8]</td></tr>
<tr><td>Address 0x04</td><td colspan=8 align=center>ADCV3:[7..0]</td></tr>
<tr><td>Address 0x05</td><td colspan=8 align=center>ADCV3:[15..8]</td></tr>
<tr><td>Address 0x06</td><td colspan=8 align=center>ADCV4:[7..0]</td></tr>
<tr><td>Address 0x07</td><td colspan=8 align=center>ADCV4:[15..8]</td></tr>
<tr><td>Access</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td></tr>
<tr><td>Default</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td></tr>
</table>

ADCV\[0,2..4\] - value of the last ADC result on port P\[0,2..4\], 2 bytes, the LSB first. Always 16-bit value: regardless of the resolution (RES), the result is always left-shifted so that the most significant bit is in the 15th position, the least significant bits are cleared.

**Addresses 0x08, 0x0a, 0x0c, 0x0e**

<table>
<tr><td>Bit</td><td>7</td><td>6</td><td>5</td><td>4</td><td>3</td><td>2</td><td>1</td><td>0</td></tr>
<tr><td>Address 0x08</td><td>OE0</td><td>OC0</td><td>DI0</td><td>REF0</td><td colspan=4 align=center>RES0:[3..0]</td></tr>
<tr><td>Address 0x0a</td><td>OE2</td><td>OC2</td><td>DI2</td><td>REF2</td><td colspan=4 align=center>RES2:[3..0]</td></tr>
<tr><td>Address 0x0c</td><td>OE3</td><td>OC3</td><td>DI3</td><td>REF3</td><td colspan=4 align=center>RES3:[3..0]</td></tr>
<tr><td>Address 0x0e</td><td>OE4</td><td>OC4</td><td>DI4</td><td>REF4</td><td colspan=4 align=center>RES4:[3..0]</td></tr>
<tr><td>Access</td><td>R/W</td><td>R/W</td><td>R</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td></tr>
<tr><td>Default</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td></tr>
</table>

OE\[0,2..4\] and OC\[0,2..4\] - port P\[0,2..4\] operation mode:  

| OE  | OC  | Port mode |
| --- | --- | --- |
| 0   | 0   | High impedance state |
| 0   | 1   | A pull-up resistor is connected between input and VCC |
| 1   | 0   | Output logical 0 (low) |
| 1   | 1   | Output logical 1 (high) |

Setting output port mode does not disable input capability (reading/monitoring logical level or ADC function) of port.

DI\[0..4\] (bit DI1 for port P1 is located in byte at address 0x1f) - current logical level read from port P\[0..4\], 0 - low, 1 - high.

REF\[0,2..4\] - ADC reference voltage for port P\[0,2..4\], will be used for subsequent ADC. Indicates the conversion range for the ADC, any exceeding voltage will result in maximum ADC value.

| REF | Reference voltage |
| --- | --- |
| 0   | VCC |
| 1   | Internal 1.1V |

RES\[0,2..4\] - ADC resolution in bits (from 1 to 15, value 0 corresponds to 16 bits) for port P\[0,2..4\], will be used for subsequent ADC.

**Addresses 0x09, 0x0b, 0x0d, 0x0f**

<table>
<tr><td>Bit</td><td>7</td><td>6</td><td>5</td><td>4</td><td>3</td><td>2</td><td>1</td><td>0</td></tr>
<tr><td>Address 0x09</td><td>POR</td><td>AFD0</td><td>AFH0</td><td>AFL0</td><td>AEH0</td><td>AEL0</td><td>AED0</td><td>AWD0</td></tr>
<tr><td>Address 0x0b</td><td>POR</td><td>AFD2</td><td>AFH2</td><td>AFL2</td><td>AEH2</td><td>AEL2</td><td>AED2</td><td>AWD2</td></tr>
<tr><td>Address 0x0d</td><td>POR</td><td>AFD3</td><td>AFH3</td><td>AFL3</td><td>AEH3</td><td>AEL3</td><td>AED3</td><td>AWD3</td></tr>
<tr><td>Address 0x0f</td><td>POR</td><td>AFD4</td><td>AFH4</td><td>AFL4</td><td>AEH4</td><td>AEL4</td><td>AED4</td><td>AWD4</td></tr>
<tr><td>Access</td><td>R/W</td><td>R/C</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td></tr>
<tr><td>Default</td><td>1</td><td>0</td><td>0</td><td>0</td><td>1</td><td>1</td><td>0</td><td>0</td></tr>
</table>

POR - power-on reset flag, 0 - cleared, 1 - set. The device itself can only set this flag, which means that all settings have been reset to default. User can write 0 or 1 (but writing 1 will not generate a reset cycle). Flag is updated simultaneously in all four bytes.

AFD\[0..4\] (bit AFD1 for port P1 is located in byte at address 0x1f, but in a bit with a different number!) - logical level alarm flag of port P\[0..4\], 0 - cleared, 1 - set. By the user this flag can only be cleared by writing 1, writing 0 keeps flag unchanged:  

| Writing to AFD | AFD |
| --- | --- |
| 0   | unchanged |
| 1   | 0   |

By the device itself this flag can only be set and only if corresponding alarm enable bit AED\[0..4\] is set to 1 and the logical level of port P\[0..4\] (0 - low, 1 - high) matches the corresponding value of the AWD\[0..4\] bit, otherwise flag remains unchanged:  

| AED | AWD | Port logical level | AFD |
| --- | --- | --- | --- |
| 0   | any | any | unchanged |
| 1   | 0   | always high | unchanged |
| 1   | 0   | detected low | 1   |
| 1   | 1   | always low | unchanged |
| 1   | 1   | detected high | 1   |

AFL\[0,2..4\] / AFH\[0,2..4\] - analog value too low/high (below THL\[0,2..4\] / above THH\[0,2..4\] threshold) alarm flag, 0 - cleared, 1 - set. User can write 0 or 1. By the device itself this flag is set only if the corresponding alarm enable bit AEL\[0,2..4\] / AEH\[0,2..4\] is set to 1 and ADC result is outside the corresponding threshold, otherwise flag is cleared. Both flags are updated after the ADC is completed and ADCV\[0,2..4\] is written for the corresponding port, the flags of the port for which ADC was not performed remain unchanged:

| ADC was performed on port | AEL | ADC result | AFL |
| --- | --- | --- | --- |
| no  | any |     | unchanged |
| yes | 0   | any | 0   |
| yes | 1   | ADCV:\[15..8\] >= THL | 0   |
| yes | 1   | ADCV:\[15..8\] < THL  | 1   |

| ADC was performed on port | AEH | ADC result | AFH |
| --- | --- | --- | --- |
| no  | any |     | unchanged |
| yes | 0   | any | 0   |
| yes | 1   | ADCV:\[15..8\] <= THH | 0   |
| yes | 1   | ADCV:\[15..8\] > THH  | 1   |

When comparing to the threshold, only the high byte of the ADC result is used, low byte is completely ignored.

AEL\[0,2..4\] / AEH\[0,2..4\] - enable analog low/high alarm for port P\[0,2..4\] (see tables of states in AFL\[0,2..4\] / AFH\[0,2..4\] bits description).

AED\[0..4\] (bit AED1 for port P1 is located in byte at address 0x1f) - enable logical level alarm for port P\[0..4\] (see table of states in AFD\[0..4\] bit description).

AWD\[0..4\] (bit AWD1 for port P1 is located in byte at address 0x1f) - monitored logical level of port P\[0..4\] (0 - low, 1 - high), upon detection of which the alarm flag is set (see table of states in AFD\[0..4\] bit description).

**Addresses 0x10, 0x12, 0x14, 0x16**

<table>
<tr><td>Bit</td><td>7</td><td>6</td><td>5</td><td>4</td><td>3</td><td>2</td><td>1</td><td>0</td></tr>
<tr><td>Address 0x10</td><td colspan=8 align=center>THL0:[7..0]</td></tr>
<tr><td>Address 0x12</td><td colspan=8 align=center>THL2:[7..0]</td></tr>
<tr><td>Address 0x14</td><td colspan=8 align=center>THL3:[7..0]</td></tr>
<tr><td>Address 0x16</td><td colspan=8 align=center>THL4:[7..0]</td></tr>
<tr><td>Access</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td></tr>
<tr><td>Default</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td></tr>
</table>

THL\[0,2..4\] - low threshold value for the analog signal on port P\[0,2..4\], if ADC result is lower than this value, alarm flag is set (see table of states in AFL\[0,2..4\] / AFH\[0,2..4\] bits description).

**Addresses 0x11, 0x13, 0x15, 0x17**

<table>
<tr><td>Bit</td><td>7</td><td>6</td><td>5</td><td>4</td><td>3</td><td>2</td><td>1</td><td>0</td></tr>
<tr><td>Address 0x11</td><td colspan=8 align=center>THH0:[7..0]</td></tr>
<tr><td>Address 0x13</td><td colspan=8 align=center>THH2:[7..0]</td></tr>
<tr><td>Address 0x15</td><td colspan=8 align=center>THH3:[7..0]</td></tr>
<tr><td>Address 0x17</td><td colspan=8 align=center>THH4:[7..0]</td></tr>
<tr><td>Access</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td></tr>
<tr><td>Default</td><td>1</td><td>1</td><td>1</td><td>1</td><td>1</td><td>1</td><td>1</td><td>1</td></tr>
</table>

THH\[0,2..4\] - high threshold value for the analog signal on port P\[0,2..4\], if ADC result is higher than this value, alarm flag is set (see table of states in AFL\[0,2..4\] / AFH\[0,2..4\] bits description).

**Address 0x1e**

<table>
<tr><td>Bit</td><td>7</td><td>6</td><td>5</td><td>4</td><td>3</td><td>2</td><td>1</td><td>0</td></tr>
<tr><td>Address 0x1e</td><td colspan=8 align=center>PWM1:[7..0]</td></tr>
<tr><td>Access</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td></tr>
<tr><td>Default</td><td>1</td><td>1</td><td>1</td><td>1</td><td>1</td><td>1</td><td>1</td><td>1</td></tr>
</table>

PWM1 - sets the duty cycle of signal on port P1 in PWM mode (OE1 = 1, OC1 = 1), in other modes the parameter does not affect anything. The resulting duty cycle will be: S = (PWM1 + 1) / 256. The duty cycle S = 0 can be achieved by setting port mode OE1 = 1, OC1 = 0, i.e. output logical 0 (low). Setting PWM1 = 255 (default value), OE1 = 1, OC1 = 1 corresponds to output logical 1 (high).

**Address 0x1f**

<table>
<tr><td>Bit</td><td>7</td><td>6</td><td>5</td><td>4</td><td>3</td><td>2</td><td>1</td><td>0</td></tr>
<tr><td>Address 0x1f</td><td>OE1</td><td>OC1</td><td>DI1</td><td>unused</td><td>unused</td><td>AFD1</td><td>AED1</td><td>AWD1</td></tr>
<tr><td>Access</td><td>R/W</td><td>R/W</td><td>R</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td></tr>
<tr><td>Default</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td></tr>
</table>

OE1 and OC1 - port P1 operation mode:  

| OE  | OC  | Port mode |
| --- | --- | --- |
| 0   | 0   | High impedacne state |
| 0   | 1   | A pull-up resistor is connected between input and VCC |
| 1   | 0   | Output logical 0 (low) |
| 1   | 1   | Output PWM signal |

Setting output port mode does not disable input capability (reading/monitoring logical level) of port.

DI1, AFD1, AED1, AWD1 - see DI\[0..4\], AFD\[0..4\], AED\[0..4\], AWD\[0..4\] bits description respectively.

**User data**

<table>
<tr><td>Bit</td><td>7</td><td>6</td><td>5</td><td>4</td><td>3</td><td>2</td><td>1</td><td>0</td></tr>
<tr><td>Address 0x18</td><td colspan=8 align=center>unused</td></tr>
<tr><td>Address 0x19</td><td colspan=8 align=center>unused</td></tr>
<tr><td>Address 0x1a</td><td colspan=8 align=center>unused</td></tr>
<tr><td>Address 0x1b</td><td colspan=8 align=center>unused</td></tr>
<tr><td>Address 0x1c</td><td colspan=8 align=center>unused</td></tr>
<tr><td>Address 0x1d</td><td colspan=8 align=center>unused</td></tr>
<tr><td>Access</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td><td>R/W</td></tr>
<tr><td>Default</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td><td>0</td></tr>
</table>

Addresses are not used by the device, they can be used to store user data.

