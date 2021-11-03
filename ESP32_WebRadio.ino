//
// ESP32 WebRadio
// 
// Released under GNU General Public License v3.0
//
// Compile with DOIT ESP32 WebKit v1 and burn /data directory into SPIFFS (1M)
//
// 0.0.1 - 08.05.2020 
//  - First public release
//
// 0.0.2 - 09.05.2020
//  - Added streams.json with lists of stream URL to be played
//  - Some minon bugs fixed
//
// 0.0.3 - 14.05.2020
//  - Some design changes, like 3 mechanical buttons (but you can use capacitive one, if you want)
//  - Streams list also on webpage, with the ability to play each with a click
//  - Display graphic improvementes
//  - Lots of new features and bugs fixed
//  - Other minor changes
//
// 0.0.4 - 20.05.2020
//  - Better OTA handling
//  - Display now show scrolling title and stream name, current time and wifi power in top bar
//
// 0.0.5 - 03.06.2020
//  - Better wifi icon
//  - Display now show more info about radio and songs
//  - Adding streams URL via Web now work 
//  - Minor bugs fixed
//
// 0.0.6 - 30.07.2020
//  -  Just small remarks to fix some compiler errors
//
// 0.0.7 - 16.0.4.2021
//  -  Just some small fixes and a refresh for library dependencies. Tested with:
//     - ESP32-audioI2S (https://github.com/schreibfaul1/ESP32-audioI2S) 783f67c
//     - ESP32-Arduino (https://github.com/espressif/arduino-esp32) 1.0.6
      
#define __DEBUG__
//#define __NEOPIXEL__ // Uncomment if you plain to use one or more NeoPixel

// Firmware data
const char BUILD[] = __DATE__ " " __TIME__;
#define FW_NAME         "esp32-webradio"
#define FW_VERSION      "0.0.7"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "time.h"

// Audio to PCM5102 DAC
// "https://github.com/schreibfaul1/ESP32-audioI2S"
#include "Audio.h"

#define I2S_DOUT      22
#define I2S_BCLK      26
#define I2S_LRC       25

#define MAX_VOLUME 21
#define MIN_VOLUME 0

Audio audio;

// Control buttons
#define BUTTONS_NUM 3
const int BUTTONS_PINS[BUTTONS_NUM] = {34, 35, 32};

// Icons
#define audio_icon_width 5
#define audio_icon_height 10
static const uint8_t PROGMEM audio_icon_bits[] = {
   0x10, 0x18, 0x1c, 0x1f, 0x1f, 0x1f, 0x1f, 0x1c, 0x18, 0x10 };

#define wifi_icon_width 8
#define wifi_icon_height 8
static const uint8_t PROGMEM wifi_icon_bits[] = {
  0x00, 0x1f, 0x20, 0x4e, 0x50, 0x56, 0x56, 0x40 };


// LCD PCD8544 - NOKIA 5110
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <Fonts/Org_01.h>

#define DISPLAY_SCLK 23
#define DISPLAY_DIN 19
#define DISPLAY_DC 21
#define DISPLAY_CS 18 // CE
#define DISPLAY_RST 14

#define DISPLAY_BL 27
#define DISPLAY_BL_DEFAULT 100 // Backlight LEDs PWM duty-cycle

// Serial clock out (SCLK), Serial data out (DIN), Data/Command select (D/C), LCD chip select (CS), LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(DISPLAY_SCLK, DISPLAY_DIN, DISPLAY_DC, DISPLAY_CS, DISPLAY_RST);

#ifdef __NEOPIXEL__

// Neopixel
// https://github.com/adafruit/Adafruit_NeoPixel
#include <Adafruit_NeoPixel.h>

#define PIN        2 
#define NUMPIXELS  1 // Popular NeoPixel ring size
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#endif

// WebServer
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

// ArduinoJson
// https://arduinojson.org/
#include <ArduinoJson.h>

// File System
#include <FS.h>   
#include "SPIFFS.h"

// Format SPIFFS if mount failed
#define FORMAT_SPIFFS_IF_FAILED 1

#include "soc/soc.h" //disable brownour problems
#include "soc/rtc_cntl_reg.h" //disable brownour problems

// Config
struct Config {
  // WiFi config
  char wifi_essid[24];
  char wifi_password[16];
  // NTP Config
  char ntp_server[16];
  int8_t ntp_timezone;
  int8_t ntp_daylightOffset;
  // Host config
  char hostname[16];
  bool ota_enable;
  char ota_password[8];
  // Radio podcast
  int8_t stream_id;
  int8_t stream_count;
  char stream_url[64];
  int8_t volume;
  // Display config
  int8_t contrast;
};

#define CONFIG_FILE "/config.json"
File configFile;
Config config; // Global config object

// JSON file contains an array of streams URL
#define DB_FILE "/streams.json"

static int last; // millis counter
bool streamIsValid = false;
bool streamChanged = true;
bool streamIsPaused = false;
bool streamIsPlaying = false;
bool deviceIsOTA = false;

DynamicJsonDocument env(256);

/*
 * Esternal procedures definition
 */
bool loadConfigFile();
void initWebServer();
void printStreamsDB();
bool getStreamURL(uint8_t);
  
// ************************************
// DEBUG_PRINT() and DEBUG_PRINTLN()
//
// send message to Serial 
// ************************************
void DEBUG_PRINT(String message) {
#ifdef __DEBUG__
  Serial.print(message);
#endif
}

void DEBUG_PRINTLN(String message) {
#ifdef __DEBUG__
  Serial.println(message);
#endif
}

// ************************************
// connectToWifi()
//
// connect to configured WiFi network
// ************************************
bool connectToWifi() {
  uint8_t timeout=0;

  if(strlen(config.wifi_essid) > 0) {
    DEBUG_PRINT("[INIT] Connecting to "+String(config.wifi_essid)+"...");

    WiFi.begin(config.wifi_essid, config.wifi_password);

    while((WiFi.status() != WL_CONNECTED)&&(timeout < 10)) {
      delay(250);
      timeout++;
    }
    if(WiFi.status() == WL_CONNECTED) {
      DEBUG_PRINTLN("CONNECTED. IP:"+WiFi.localIP().toString()+" GW:"+WiFi.gatewayIP().toString());

      env["ip"] = WiFi.localIP().toString();
      if (MDNS.begin(config.hostname)) {
        DEBUG_PRINTLN("[INIT] MDNS responder started: "+String(config.hostname));
        // Add service to MDNS-SD
        MDNS.addService("http", "tcp", 80);
      }
      
      configTime(config.ntp_timezone * 3600, config.ntp_daylightOffset * 3600, config.ntp_server);
      
      return true;  
    } else {
      DEBUG_PRINTLN("[ERROR] Failed to connect to WiFi");
      return false;
    }
  } else {
    DEBUG_PRINTLN("[ERROR] Please configure Wifi");
    return false; 
  }
}

//
// high priority audioTask on CORE0
//
void audioTask(void *pvParameters) {
  while(1) {
    if(!streamIsPaused) {
      if(streamChanged) {
        playStream(config.stream_id);
        streamChanged=false;
      }
      audio.loop();  
    
      streamIsPlaying = audio.isRunning();
    }
  }
}

//
// SETUP()
//
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.print("ESP32 SDK: ");
  Serial.println(ESP.getSdkVersion());
  Serial.println();
  Serial.print(FW_NAME);
  Serial.print(" ");
  Serial.print(FW_VERSION);
  Serial.print(" ");
  Serial.println(BUILD);
  delay(1000);

  // Initialize SPIFFS
  DEBUG_PRINT("[INIT] Initializing SPIFFS...");
  if(!SPIFFS.begin()){
    DEBUG_PRINTLN("[ERROR] SPIFFS mount failed. Try formatting...");
    if(SPIFFS.format()) {
      DEBUG_PRINTLN("[INIT] SPIFFS initialized successfully");
    } else {
      DEBUG_PRINTLN("[FATAL] SPIFFS fatal error");
      ESP.restart();
    }
  } else {
    DEBUG_PRINTLN("OK");
  }

  // Initialize buttons control board
  for (int i = 0; i < BUTTONS_NUM; i++) {
    pinMode(BUTTONS_PINS[i], INPUT);
  }  

  // Load configuration
  loadConfigFile();
    
  // Initialize DISPLAY
  DEBUG_PRINT("[INIT] Initialize display...");

  display.begin();
  display.setContrast(100);
  
  pinMode(DISPLAY_BL,OUTPUT);
  ledcSetup(0, 12000, 8);
  ledcAttachPin(DISPLAY_BL, 0);
  // Fade-in
  for(uint8_t i=0;i<DISPLAY_BL_DEFAULT;i++) {
    ledcWrite(0, i);
    delay(10);
  }
  // A tiny,stylized font with all characters within a 6 pixel height.
  display.setFont(&Org_01);
  
  display.display();
  DEBUG_PRINTLN("OK");

  // Connect to WiFi network
  connectToWifi();

  // Initialize Web Server
  initWebServer();

  // Initialize OTA
  if(config.ota_enable) {
    DEBUG_PRINT("[INIT] Initialize OTA...");
    // ArduinoOTA.setPort(3232);
    ArduinoOTA.setHostname(config.hostname);

    ArduinoOTA
      .onStart([]() {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
          type = "sketch";
        else // U_SPIFFS
          type = "filesystem";
        Serial.println("Start updating " + type);
        deviceIsOTA=true;
        streamIsPaused=true;
      })
      .onEnd([]() {
        Serial.println("\nEnd");
        deviceIsOTA=false;
        streamIsPaused=false;
      })
      .onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        display.setCursor(8,24);
        display.printf("Progress: %u%%\r", (progress / (total / 100)));    
      })
      .onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
        deviceIsOTA=false;
      });
    ArduinoOTA.begin();
    DEBUG_PRINTLN("OK");
  }
  //
  DEBUG_PRINT("[INIT] Initialize audio...");

  env["volume"] = config.volume;

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(env["volume"].as<int>());
  DEBUG_PRINTLN("OK");

#ifdef __NEOPIXEL__
  DEBUG_PRINT("[INIT] Initialize NeoPixel...");
  pixels.begin();
  pixels.setBrightness(80);
  pixels.show(); // Initialize all pixels to 'off'
  DEBUG_PRINTLN("OK");
#endif

  //
  printStreamsDB();

  // let's fly!
  DEBUG_PRINTLN("[INIT] Ready: let's fly!");
  env["status"] = "Ready";

  // Launch audioTask on CORE0 with high priority
  disableCore0WDT(); // Disable watchdog on CORE0
  xTaskCreatePinnedToCore(
                  audioTask,   /* Function to implement the task */
                  "audioTask", /* Name of the task */
                  10000,      /* Stack size in words */
                  NULL,       /* Task input parameter */
                  15,          /* Priority of the task */
                  NULL,       /* Task handle. */
                  0);  /* Core where the task should run */
}

String zeroPadding(int digit) {
  if(digit < 10) {
    return String("0"+String(digit));
  }
  return String(digit);
}

uint8_t t_idx=0, d_cycle=0, d_cnt=0;

void updateDisplay() {
  d_cnt++;
  if((d_cnt % 5)==0) {
    d_cycle++;
    if(d_cycle > 2) {
      d_cycle=0;
    }
  }
  
  // Graphic display of 84×48 pixels
  display.clearDisplay();
  display.setContrast(config.contrast);

  // If there's an OTA running...
  if(deviceIsOTA) {
    display.setCursor(2,16);  
    display.println("Updating.Please wait!");       
    display.display();
    return;
  }

  display.setTextSize(1);
  // Top black BAR with WiFi signal power
  display.fillRect(0, 0, 84, 8, BLACK);
  display.drawXBitmap(0, 0, wifi_icon_bits, wifi_icon_width, wifi_icon_height, 0x00);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(12,6);
  display.println(String(WiFi.RSSI())+"dB");

  display.setCursor(48,6);
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    DEBUG_PRINTLN("[TIME] Failed to obtain time");
  } else {
    display.println(&timeinfo, "%H:%M:%S");
  }
  
  // Display stream name and song title (scrolling)
  display.setTextColor(BLACK, WHITE);
  display.setCursor(4,16);  
  String title = env["stream_station"].as<String>();
    
  if(env["stream_title"]) {
    title = title + " - "+env["stream_title"].as<String>()+"  ";
  }

  if(streamIsPlaying) {
    if(title.length() > 0) {
      display.println(title.substring(t_idx, t_idx+15).c_str());    
      t_idx++;
      if((t_idx+15) > title.length()) {
        t_idx=0;
      }
    }
    display.setCursor(2,24);  
    display.println(env["stream_genre"].as<String>());    
    display.setCursor(4,34);  
    switch(d_cycle) {
      case 0:
        display.println(env["stream_bitrate"].as<String>()+" brate");    
        break;
      case 1:
        display.println(env["stream_channels"].as<String>()+" chans");    
        break;
      case 2:
        display.println(env["stream_bitspersample"].as<String>()+" bps");    
        break;
      default:
        d_cycle=0;        
    }
   } else {
    display.setCursor(2,24);  
    if(streamIsPaused) {
      display.println("*** PAUSE ***");    
    } else {
      display.println("Loading...");                  
    }
  }

  // Draw volume bar
  display.drawXBitmap(2, 38, audio_icon_bits, audio_icon_width, audio_icon_height, 0xff);
  uint8_t volume=map(config.volume, 0, 22, 0, 72);
  display.drawRect(10, 40, 72, 6, BLACK);
  display.fillRect(10, 40, volume, 6, BLACK);
  //
  
  display.display();
}

bool playStream(uint8_t stream_id) {
  if(getStreamURL(stream_id)) {
    streamIsPlaying = audio.connecttohost(config.stream_url);
    return true;
  } 
  DEBUG_PRINTLN("[AUDIO] No STREAM URL defined!");
  env["status"] = "No stream URL defined at index "+stream_id;  
  return false;
}

void audio_info(const char *info){
    String sinfo=String(info);
    
    if(sinfo.startsWith("Bitrate=")) {
      env["stream_bitrate"] = sinfo.substring(8).toInt();
    } else if(sinfo.startsWith("StreamTitle=")) {
      env["stream_title"] = sinfo.substring(12,44); // Take only the first 32 chars (max)
    } else if(sinfo.startsWith("Channels=")) {
      env["stream_channels"] = sinfo.substring(8).toInt();
    } else if(sinfo.startsWith("SampleRate=")) {
      env["stream_samplerate"] = sinfo.substring(11).toInt();
    } else if(sinfo.startsWith("BitsPerSample=")) {
      env["stream_bitspersample"] = sinfo.substring(14).toInt();
    } else if(sinfo.startsWith("icy-genre:")) { // Get the stream genre, if present
      env["stream_genre"] = sinfo.substring(10,24);
    } else {
      DEBUG_PRINT("[AUDIO] "); DEBUG_PRINTLN(info);
    }
}

void audio_showstation(const char *info){
  String sinfo=String(info);
  env["stream_station"] = sinfo.substring(0,16); // First 16 chars only (max)
}

// Stream actions
void nextStream() {
  config.stream_id++;
  if(config.stream_id >= config.stream_count) {
    config.stream_id=0;
  }
  streamChanged=true;
}

void togglePlay() {
  streamIsPlaying = audio.pauseResume();
  DEBUG_PRINT("[AUDIO] pauseResume(): ");
  if(streamIsPlaying) {
    DEBUG_PRINTLN("PLAY");
    streamIsPaused = false;
    env["status"] = "Playing "+config.stream_id;
  } else {
    DEBUG_PRINTLN("PAUSE");
    streamIsPaused = true;
    env["status"] = "Pause";
  }
}

// Control buttons
uint8_t buttons=0;

void checkButtons() {
  for (int i = 0; i < BUTTONS_NUM; i++) { 
    if(digitalRead(BUTTONS_PINS[i])) {
      buttons |= 1 << i;
    } else {
      buttons &= ~(1 << i);
    }
  }
  
  if(buttons) {
    DEBUG_PRINT("[BUTTON] Pressed ");
    DEBUG_PRINTLN(String(buttons));
    switch(buttons) {
      case 1: // BUTTONS 1 - Volume UP
        if(config.volume < MAX_VOLUME) {
          config.volume = config.volume+1;
        }
        break;
      case 2: // BUTTONS 2 - Volume DOWN
        if(config.volume > MIN_VOLUME) {
          config.volume = config.volume-1;
        }        
        break;
      case 3: // BUTTONS 1+2 - Play/Pause
        togglePlay();        
        break;
      case 4: // BUTTONS 3 - Next stream
        nextStream();
        break;
      case 5: // BUTTONS 3+1
        break;
      case 6: // BUTTONS 2+3
        break;
      case 7: // BUTTONS 1+2+3
        break;
      default:
        break;
    }
  }
}

uint8_t tickCounter=0;

void loop() {    
  if(config.ota_enable) {
    ArduinoOTA.handle();
  }
  
  if((millis() - last) > 1100) {  
    checkButtons();

    if(WiFi.status() != WL_CONNECTED) {
      // Not connected? RETRY! 
      connectToWifi();
    }  

    audio.setVolume(config.volume);     
    env["volume"] = config.volume;

    tickCounter++;
    if((tickCounter % 10)==0) {
      if(!streamIsPlaying) {
        playStream(config.stream_id);
      }
    }

    updateDisplay();
    last = millis();    
  }
}
