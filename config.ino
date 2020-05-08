// Written by Michele<o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty

#include <ArduinoJson.h>
// File System
#include <FS.h>   
#include "SPIFFS.h"

// ************************************
// Config, save and load functions
//
// save and load configuration from config file in SPIFFS. JSON format (need ArduinoJson library)
// ************************************
bool loadConfigFile() {
  DynamicJsonDocument root(512);
#ifdef __DEBUG__
  Serial.println("[DEBUG] loadConfigFile()");
#endif
  configFile = SPIFFS.open(CONFIG_FILE, "r");
  if (!configFile) {
    DEBUG_PRINT("[CONFIG] Config file not available");
    return false;
  } else {
    // Get the root object in the document
    DeserializationError err = deserializeJson(root, configFile);
    if (err) {
      DEBUG_PRINT("[CONFIG] Failed to read config file:"+String(err.c_str()));
      return false;
    } else {
      strlcpy(config.wifi_essid, root["wifi_essid"], sizeof(config.wifi_essid));
      strlcpy(config.wifi_password, root["wifi_password"], sizeof(config.wifi_password));
      strlcpy(config.hostname, root["hostname"] | "aiq-sensor", sizeof(config.hostname));
      strlcpy(config.ntp_server, root["ntp_server"] | "time.ien.it", sizeof(config.ntp_server));
      config.ntp_timezone = root["ntp_timezone"] | 1;
      strlcpy(config.stream_url, root["stream_url"] | "", sizeof(config.stream_url));
      config.volume = root["volume"] | 20;
      config.contrast = root["contrast"] | 50;

      DEBUG_PRINT("[CONFIG] Configuration loaded");
    }
  }
  configFile.close();
  return true;
}

bool saveConfigFile() {
  DynamicJsonDocument root(512);
  DEBUG_PRINT("[DEBUG] saveConfigFile()");

  root["wifi_essid"] = config.wifi_essid;
  root["wifi_password"] = config.wifi_password;
  root["hostname"] = config.hostname;
  root["ntp_server"] = config.ntp_server;
  root["ntp_timezone"] = config.ntp_timezone;
  root["stream_url"] = config.stream_url;
  root["volume"] = config.volume;
  root["contrast"] = config.contrast;
  
  configFile = SPIFFS.open(CONFIG_FILE, "w");
  if(!configFile) {
    DEBUG_PRINT("[CONFIG] Failed to create config file !");
    return false;
  }
  serializeJson(root,configFile);
  configFile.close();
  return true;
}
