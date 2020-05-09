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
  
  DEBUG_PRINT("[DEBUG] loadConfigFile()");
  
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
      config.stream_id = root["stream_id"] | 0;
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
  root["stream_id"] = config.stream_id;
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

// Debug function that return stream URLs list
void printStreamsDB() {
  DynamicJsonDocument doc(512);
  
  DEBUG_PRINT("[DEBUG] printStreamDB()");
  
  File dbFile = SPIFFS.open(DB_FILE, "r");
  if (!dbFile) {
    DEBUG_PRINT("[DB] DB file not found");
    return;
  }
  
  // Get the root object in the document
  DeserializationError err = deserializeJson(doc, dbFile);
  if (err) {
    DEBUG_PRINT("[DB] Failed to read DB file:"+String(err.c_str()));
    return;
  }

  JsonArray streams = doc["streams"];
// streams[0] => "http://stream.dancewave.online:8080/dance.mp3"
  
  DEBUG_PRINT("DB memory usage:"+String(streams.memoryUsage())+" Bytes");

  for(uint8_t i=0;i<streams.size();i++) {
    DEBUG_PRINT(streams[i]);
  }
}

bool getStreamURL(uint8_t id) {
  DynamicJsonDocument doc(512);
  
  DEBUG_PRINT("[DEBUG] getStreamUrl("+String(id)+")");
  
  File dbFile = SPIFFS.open(DB_FILE, "r");
  if (!dbFile) {
    DEBUG_PRINT("[DB] DB file not found");
    return false;
  }
  
  // Get the root object in the document
  DeserializationError err = deserializeJson(doc, dbFile);
  if (err) {
    DEBUG_PRINT("[DB] Failed to read DB file:"+String(err.c_str()));
    return false;
  }

  JsonArray streams = doc["streams"];

  config.stream_count = streams.size();

  if(id < streams.size()) {
    strncpy(config.stream_url,streams[id],64);
    DEBUG_PRINT("[DB] Loaded "+String(config.stream_url));
    return true;
  } else {
    return false;
  }
}
