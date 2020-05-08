//
// ESP32 WebRadio
// 
// Compile with DOIT ESP32 WebKit v1
//
// 0.0.1 - 08.05.2020 - First public release
// 
      
#define __DEBUG__

// Firmware data
const char BUILD[] = __DATE__ " " __TIME__;
#define FW_NAME         "esp32-webradio"
#define FW_VERSION      "0.0.1"

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

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

// CJMCU-0401 Capacitive touch buttons
const int BUTTON_PINS[4] = {34, 35, 32, 33};

// LCD PCD8544 - NOKIA 5110
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

#define DISPLAY_SCLK 23
#define DISPLAY_DIN 19
#define DISPLAY_DC 21
#define DISPLAY_CS 18 // CE
#define DISPLAY_RST 14

#define DISPLAY_BL 27

// Serial clock out (SCLK), Serial data out (DIN), Data/Command select (D/C), LCD chip select (CS), LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(DISPLAY_SCLK, DISPLAY_DIN, DISPLAY_DC, DISPLAY_CS, DISPLAY_RST);

// Neopixel
// https://github.com/adafruit/Adafruit_NeoPixel
#include <Adafruit_NeoPixel.h>

#define PIN        2 
#define NUMPIXELS  1 // Popular NeoPixel ring size
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

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
  // Host config
  char hostname[16];
  // Radio podcast
  char stream_url[64];
  int8_t volume;
  // Display config
  int8_t contrast;
};

#define CONFIG_FILE "/config.json"

File configFile;
Config config; // Global config object

static int last; // millis counter
bool streamChanged=true;

DynamicJsonDocument env(256);
  
// ************************************
// DEBUG_PRINT()
//
// send message via RSyslog (if enabled) or fallback on Serial 
// ************************************
void DEBUG_PRINT(String message) {
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
    DEBUG_PRINT("[INIT] Connecting to "+String(config.wifi_essid));

    WiFi.begin(config.wifi_essid, config.wifi_password);

    while((WiFi.status() != WL_CONNECTED)&&(timeout < 10)) {
      delay(250);
      timeout++;
    }
    if(WiFi.status() == WL_CONNECTED) {
      DEBUG_PRINT("CONNECTED. IP:"+WiFi.localIP().toString()+" GW:"+WiFi.gatewayIP().toString());

      env["ip"] = WiFi.localIP().toString();
      if (MDNS.begin(config.hostname)) {
        DEBUG_PRINT("[INIT] MDNS responder started");
        // Add service to MDNS-SD
        MDNS.addService("http", "tcp", 80);
      }
      
      configTime(config.ntp_timezone * 3600, config.ntp_timezone * 3600, config.ntp_server);
      
      return true;  
    } else {
      DEBUG_PRINT("[ERROR] Failed to connect to WiFi");
      return false;
    }
  } else {
    DEBUG_PRINT("[ERROR] Please configure Wifi");
    return false; 
  }
}

//
// high priority audioTask on CORE0
//
void audioTask(void *pvParameters) {
  while(1) {
    audio.loop();  
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
  if(!SPIFFS.begin()){
    DEBUG_PRINT("[ERROR] SPIFFS mount failed. Try formatting...");
    if(SPIFFS.format()) {
      DEBUG_PRINT("[INIT] SPIFFS initialized successfully");
    } else {
      DEBUG_PRINT("[FATAL] SPIFFS fatal error");
      ESP.restart();
    }
  } else {
    DEBUG_PRINT("[INIT] SPIFFS OK");
  }

  // Initialize 4-buttons capacitive board
  for (int i = 0; i < 4; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
  }  

  // Load configuration
  loadConfigFile();
    
  // Initialize DISPLAY
  display.begin();
  display.setContrast(config.contrast);
  
  pinMode(DISPLAY_BL,OUTPUT);
  ledcSetup(0, 1000, 8);
  ledcAttachPin(DISPLAY_BL, 0);
  ledcWrite(0, 100);
  
  display.display();

  // Connect to WiFi network
  connectToWifi();

  // Initialize Web Server
  initWebServer();

  env["volume"] = config.volume;

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(env["volume"].as<int>());
  audio.connecttohost(config.stream_url);

  // Set BOOL value to load and play stream...
  streamChanged=true;

  pixels.begin();
  pixels.setBrightness(80);
  pixels.show(); // Initialize all pixels to 'off'

  // let's fly!
  DEBUG_PRINT("[INIT] Ready: let's fly!");
  env["status"] = "Ready";

  // Launch audioTask on CORE0 with high priority
  xTaskCreatePinnedToCore(
                    audioTask,   /* Function to implement the task */
                    "audioTask", /* Name of the task */
                    10000,      /* Stack size in words */
                    NULL,       /* Task input parameter */
                    10,          /* Priority of the task */
                    NULL,       /* Task handle. */
                    0);  /* Core where the task should run */
}

void updateDisplay() {
  display.clearDisplay();
  display.setContrast(config.contrast);

  display.setCursor(0,0);
  display.setTextSize(1);
  display.println(env["stream_station"].as<const char*>());
  
  display.setCursor(0,20);
  display.println(env["stream_title"].as<const char*>());
  
  display.display();
}

void audio_info(const char *info){
    String sinfo=String(info);
    if(sinfo.startsWith("Bitrate=")) {
      env["stream_bitrate"] = sinfo.substring(8).toInt();
    } else if(sinfo.startsWith("StreamTitle=")) {
      env["stream_title"] = sinfo.substring(12,44); // Take only the first 32 chars (max)
    } else {
      DEBUG_PRINT("audio_info: "); DEBUG_PRINT(info);
    }
}

void audio_showstation(const char *info){
  String sinfo=String(info);
  env["stream_station"] = sinfo.substring(0,16); // First 16 chars only (max)
}

uint8_t buttons=false;

void checkButtons() {
   for (int i = 0; i < 4; i++) { 
    if(digitalRead(BUTTON_PINS[i])) {
      buttons |= 1 << i;
    } else {
      buttons &= ~(1 << i);
    }
  }
}

void loop() {    
  checkButtons();

  if(buttons) {
    DEBUG_PRINT(String(buttons));
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
      case 3: // BUTTONS 1+2
        break;
      case 4: // BUTTONS 3
        break;
      case 5: // BUTTONS 3+1
        break;
      case 6: // BUTTONS 2+3
        break;
      case 7: // BUTTONS 1+2+3
        break;
      case 8: // BUTTONS 4
        break;
      case 9: // BUTTONS 4+1
        break;
      default:
        break;
    }
  }
  
  if((millis() - last) > 1100) {  
    if(WiFi.status() != WL_CONNECTED) {
      // Not connected? RETRY! 
      connectToWifi();
    }
    audio.setVolume(config.volume); 
    env["volume"] = config.volume;
    updateDisplay();
    last = millis();
  }
  delay(200);
}
