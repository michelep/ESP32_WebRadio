# ESP32_WebRadio
An Internet web radio based to ESP32

ESP32 connect to the Internet via WiFI (support PSK/PSK2), fetching MP3/AAC audio stream from your favourite webradio (mine is Dance Wave!). Then decode MP3 and send via I2S to DAC. The DAC simply output audio.

## Configuration

Edit data/config.json file with your wifi ESSID and password. Other stuff can be edited via web interface, available after successfull contection to the net. 

## Bill of materials

- DOIT ESP32 WebKit v1 (or other suitable ESP32 board...)
- CJMCU 0401 - 4 capacitive touch button module
- CJMCU PCM5102 DAC 
- PCD8544 LCD Display (aka NOKIA 5110 display) module
- One or more NeoPixel, but they are not mandatory ;-)

## Schematic
![Schematic](https://raw.githubusercontent.com/michelep/ESP32_WebRadio/master/images/schematic.png)
