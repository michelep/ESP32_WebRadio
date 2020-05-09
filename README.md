# ESP32_WebRadio
An Internet web radio based to ESP32

ESP32 connect to the Internet via WiFI (support PSK/PSK2), fetching MP3/AAC audio stream from your favourite webradio (mine is Dance Wave!). Then decode MP3 and send via I2S to DAC. The DAC simply output audio.

![ESP32 WebRadio](https://raw.githubusercontent.com/michelep/ESP32_WebRadio/master/images/esp32_webradio.jpg)

## Configuration

Edit data/config.json file with your wifi ESSID and password. Fill data/streams.json with your favourites streams. Then burn the whole data directory into SPIFFS partition. Other stuff can be edited via web interface, available after successfull contection to the net. 

## Bill of materials

- DOIT ESP32 WebKit v1 (or other suitable ESP32 board...)
- CJMCU 0401 - 4 capacitive touch button module
- CJMCU PCM5102 DAC 
- PCD8544 LCD Display (aka NOKIA 5110 display) module
- One or more NeoPixel, but they are not mandatory ;-)

## Schematic
![Schematic](https://raw.githubusercontent.com/michelep/ESP32_WebRadio/master/images/schematic.png)

## ChangeLog

0.0.1 - 08.05.2020 
  - First public release

0.0.2 - 09.05.2020
  - Added streams.json with lists of stream URL to be played
  - Some minon bugs fixed

## Contributions

This work is free and far to be perfect nor complete. Contributions and PR are welcome and strongly encouraged!
