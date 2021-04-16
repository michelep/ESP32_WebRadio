# ESP32_WebRadio
An Internet web radio based to ESP32

ESP32 connect to the Internet via WiFI (support WEP/WPA/WPA2), fetching MP3/AAC/M4A audio stream from your favourites webradio (mine is Dance Wave!). Then decode MP3 and send via I2S to DAC. The DAC simply output audio to a PAM8403 3W amplifier.

My grandpa made for me a fantastic wood case that sits perfectly on my book shelf in the linving room ;-)

![ESP32 WebRadio](https://raw.githubusercontent.com/michelep/ESP32_WebRadio/master/images/esp32_webradio_overview.jpg)

This is the inside, with the mainboard, battery, charge/discharge module, speakers and, on the right, hide by a panel, the 4 buttons control board:

![ESP32 WebRadio inside](https://raw.githubusercontent.com/michelep/ESP32_WebRadio/master/images/esp32_webradio_inside.jpg)

This is a detail of the Nokia 5110 LCD display:

![ESP32 WebRadio](https://raw.githubusercontent.com/michelep/ESP32_WebRadio/master/images/esp32_webradio_front.jpg)

## Configuration

Edit data/config.json file with your wifi ESSID and password. Fill data/streams.json with your favourites streams. Then burn the whole data directory into SPIFFS partition. Other stuff can be edited via web interface, available after successfull contection to the net. 

## Bill of materials

For a basic board:

- DOIT ESP32 WebKit v1 (or other suitable ESP32 board...) (On [AliExpress](https://it.aliexpress.com/item/4000141080480.html))
- CJMCU 5102 DAC [AliExpress](https://it.aliexpress.com/item/33023894667.html)
- PCD8544 LCD Display (aka NOKIA 5110 display) module (On [AliExpress](https://it.aliexpress.com/item/32959195226.html))
- PAM8403 2*3W Amplifier board (On [AliExpress](https://it.aliexpress.com/item/32968752490.html))
- One or more NeoPixel, but they are not mandatory ;-)
- Two 8 Ohm speaker
- 3 x 10K Ohm 1/8W resistors
- 2 x 100K Ohm trimmer
- 330 Ohm 1/8W resistor
- 860 Ohm 1/8 W resistor, only if you want to add one or more NeoPixels
- 3 push buttons (On [AliExpress](https://it.aliexpress.com/item/32995191209.html))

On mine, i've added an 18650 LiBo battery and a voltage charge/discharge regulator (on [Aliexpress](https://it.aliexpress.com/item/32824032545.html)), with key pin (connected to the fourth button) to power on and off. With this module, i can power (anche charge the battery) 
using USB. I've choosed an USB B connector (i have some lying on the table...) and it works perfecly ;-). With a fully charged battery, it can play sound at reasonable volume for about 2 hours.

## Schematic
![Schematic](https://raw.githubusercontent.com/michelep/ESP32_WebRadio/master/images/schematic.png)

## ChangeLog
0.0.7 - 16.0.4.2021
  -  Just some small fixes and a refresh for library dependencies. Tested with:
     - ESP32-audioI2S (https://github.com/schreibfaul1/ESP32-audioI2S) 783f67c
     - ESP32-Arduino (https://github.com/espressif/arduino-esp32) 1.0.6

0.0.6 - 30.07.2020
  -  Just small remarks to fix some compiler errors

0.0.5 - 03.06.2020
  - Better wifi icon
  - Display now show more info about radio and songs
  - Adding streams URL via Web now work 
  - Minor bugs fixed

0.0.4 - 20.05.2020
  - Better OTA handling 
  - Display now show scrolling title and stream name, current time and wifi power in top bar, volume in bottom (WIP)
  - Miscellaneous improvements and bug fixed

0.0.3 - 14.05.2020
  - Some design changes, like 3 mechanical buttons (but you can use capacitive ones, if you want)
  - Streams list also on webpage, with the ability to play each with a click
  - Display graphic improvementes
  - Lots of new features and bugs fixed
  - Other minor changes

0.0.2 - 09.05.2020
  - Added streams.json with lists of stream URL to be played
  - Some minon bugs fixed

0.0.1 - 08.05.2020 
  - First public release

## Other resources

Start with ESP32 pinouts from here: https://randomnerdtutorials.com/esp32-pinout-reference-gpios/

Needs the great work from Wolle for MP3 stream decoding and I2S interface: https://github.com/schreibfaul1/ESP32-audioI2S

A great tutorial for Nokia 5110 LCD displays is there: https://lastminuteengineers.com/nokia-5110-lcd-arduino-tutorial/

## Contributions

This work is free and far to be perfect nor complete. Contributions and PR are welcome and strongly encouraged!
