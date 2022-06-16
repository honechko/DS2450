# 1-wire port expander

![1-wire port expander](https://github.com/honechko/DS2450/raw/main/io_adc.jpg)

This device is based on ATtiny13A with closed-source firmware with author's
features. It is not DS2450 or its replacement, but it is developed to be
possible to use it with a lot of software written for DS2450
(such as [OWFS](https://github.com/owfs/owfs) or Linux kernel modules).

## Main usage

* digital/analog sensors read
* mosfet/relay control
* alarm systems
* communication

## Key features

* 5 true GPIO ports
* device can continuously watch for certain logic state on all ports and set alarm flags
* 1 port with PWM function
* 4 ports with ADC function (up to 12 effective bits)
* device can compare each ADC result with 2 thresholds and set alarm flags
* 1-wire slave to SPI-master bridge

## Example of web-interface

![Web-interface](https://github.com/honechko/DS2450/raw/main/Docs/control.png)

