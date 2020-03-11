<a href="https://www.hardwario.com/"><img src="https://www.hardwario.com/ci/assets/hw-logo.svg" width="200" alt="HARDWARIO Logo" align="right"></a>

# Firmware for HARDWARIO WiFi Cryptoclock

Displays the current date, time and Bitcoin price in USD.

![Photo of WiFi Cryptoclock](doc/wifi-cryptoclock.jpg)

## Components

* [Battery Module](https://shop.bigclown.com/battery-module/)
* [Core Module - NR](https://shop.bigclown.com/core-module-nr/)
* [WiFi module by chiptron.cz](https://www.tindie.com/products/chiptron/wi-fi-module-with-esp8266-for-bigclown-bare-pcb/)
* [LCD Module](https://shop.bigclown.com/lcd-module-bg/)

## WiFi module

Use ESP-12E or ESP-12F (optimized PCB antenna) with 4MB (32Mb) flash size.

### BOM

* 1x ESP8266 ESP-12F
* 1x R0805 1k0 (R1)
* 3x R0805 10k (R3, R4, R5)
* 1x CKS0805 100n X7R (C1)
* 1x CKS1812 4,7u X7R (C2)
* 1x CTS 330u D (C3)

### Solder notes

* R6, R7 - DNP (Do Not Populate)
* Solder jumpers
  * RESET with P6
  * CH_PD with P8
  * IO0 with P9 (JP2)
  * UART_RX - TX to RX0 or RX1
  * UART_TX - RX to TX0 or TX1

### ESP8266 firmware

Flash latest [ESP8266 NonOS AT firmware](https://www.espressif.com/en/support/download/at).

* Install [esptool](https://github.com/espressif/esptool)
```
sudo apt install esptool
```
* Connect WiFi module with USB to UART Bridge

| WiFi module | USB to UART Bridge |
| --- | --- |
| P0 (TX0) or P2 (TX1) | TX |
| P1 (RX0) or P3 (RX1) | RX |
| P8 | 3V3 or RTS |
| P9 | GND or DTR |
| VDD | 3V3 |
| GND | GND |
* Check right connection
```
esptool --port /dev/ttyUSB0 --baud 115200 chip_id
```
* Download and flash firmware
```
wget https://www.espressif.com/sites/default/files/ap/ESP8266_NonOS_AT_Bin_V1.7.1.zip
unzip ESP8266_NonOS_AT_Bin_V1.7.1.zip
cd ESP8266_NonOS_AT_Bin_V1.7.1
esptool --port /dev/ttyUSB0 --baud 115200 --chip esp8266 write_flash --flash_size 2MB-c1 0x00000 bin/boot_v1.7.bin 0x01000 bin/at/1024+1024/user1.2048.new.5.bin 0x1fc000 bin/esp_init_data_default_v08.bin 0xfe000 bin/blank.bin 0x1fe000 bin/blank.bin
```
* Disconnect P9 pin and check version via AT command
```
picocom -b 115200 --omap crcrlf /dev/ttyUSB0
Ctrl+a Ctrl+g (set RTS to low)
AT+GMR
Ctrl+a Ctrl+x (exit)
```

### Other boards

You can use other ESP8266 board like NodeMCU or Wemos D1 mini and connect it with wires:

| ESP8266 board | HARDWARIO Core Module |
| --- | --- |
| RX | P2 |
| TX | P3 |
| EN | P8 |
| 3V3 | VDD |
| GND | GND |

## License

This project is licensed under the [MIT License](https://opensource.org/licenses/MIT/) - see the [LICENSE](LICENSE) file for details.

---

Made with &#x2764;&nbsp; by [**HARDWARIO s.r.o.**](https://www.hardwario.com/) in the heart of Europe.
