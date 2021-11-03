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
  
  DEBUG_PRINT("[CONFIG] Loading config...");
  
  configFile = SPIFFS.open(CONFIG_FILE, "r");
  if (!configFile) {
    DEBUG_PRINTLN("ERROR: Config file not available");
    return false;
  } else {
    // Get the root object in the document
    DeserializationError err = deserializeJson(root, configFile);
    if (err) {
      DEBUG_PRINTLN("ERROR: "+String(err.c_str()));
      return false;
    } else {
      strlcpy(config.wifi_essid, root["wifi_essid"], sizeof(config.wifi_essid));
      strlcpy(config.wifi_password, root["wifi_password"], sizeof(config.wifi_password));
      strlcpy(config.hostname, root["hostname"] | "aiq-sensor", sizeof(config.hostname));
      strlcpy(config.ntp_server, root["ntp_server"] | "time.ien.it", sizeof(config.ntp_server));
      config.ntp_timezone = root["ntp_timezone"] | 1;
      config.ntp_daylightOffset = root["ntp_timezone"] | 1;
      config.stream_id = root["stream_id"] | 0;
      config.volume = root["volume"] | 20;
      config.contrast = root["contrast"] | 50;
      config.ota_enable = root["ota_enable"] | true;

      DEBUG_PRINTLN("OK");
    }
  }
  configFile.close();
  return true;
}

bool saveConfigFile() {
  DynamicJsonDocument root(512);
  DEBUG_PRINT("[CONFIG] Saving config...");

  root["wifi_essid"] = config.wifi_essid;
  root["wifi_password"] = config.wifi_password;
  root["hostname"] = config.hostname;
  root["ntp_server"] = config.ntp_server;
  root["ntp_timezone"] = config.ntp_timezone;
  root["stream_id"] = config.stream_id;
  root["volume"] = config.volume;
  root["contrast"] = config.contrast;
  root["ota_enable"] = config.ota_enable;
  
  configFile = SPIFFS.open(CONFIG_FILE, "w");
  if(!configFile) {
    DEBUG_PRINTLN("ERROR: Failed to create config file !");
    return false;
  }
  serializeJson(root,configFile);
  configFile.close();
  DEBUG_PRINTLN("OK");
  return true;
}

// Debug function that return stream URLs list
void printStreamsDB() {
  DynamicJsonDocument doc(512);
  
  DEBUG_PRINT("[DB] printStreamDB()");
  
  File dbFile = SPIFFS.open(DB_FILE, "r");
  if (!dbFile) {
    DEBUG_PRINTLN("ERROR: file not found");
    return;
  }
  
  // Get the root object in the document
  DeserializationError err = deserializeJson(doc, dbFile);
  if (err) {
    DEBUG_PRINTLN("[ERROR: "+String(err.c_str()));
    return;
  }

  JsonArray streams = doc["streams"];
// streams[0] => "http://stream.dancewave.online:8080/dance.mp3"
  
  DEBUG_PRINTLN("OK. DB memory usage:"+String(streams.memoryUsage())+" bytes");

  for(uint8_t i=0;i<streams.size();i++) {
    DEBUG_PRINT("["+String(i)+"] ");
    DEBUG_PRINTLN(streams[i]);
  }
}

bool getStreamURL(uint8_t id) {
  DynamicJsonDocument doc(512);
  
  DEBUG_PRINT("[DB] getStreamUrl("+String(id)+")...");
  
  File dbFile = SPIFFS.open(DB_FILE, "r");
  if (!dbFile) {
    DEBUG_PRINTLN("ERROR: DB file not found");
    return false;
  }
  
  // Get the root object in the document
  DeserializationError err = deserializeJson(doc, dbFile);
  if (err) {
    DEBUG_PRINTLN("ERROR: "+String(err.c_str()));
    return false;
  }

  JsonArray streams = doc["streams"];

  config.stream_count = streams.size();

  if(id < streams.size()) {
    strncpy(config.stream_url,streams[id],64);
    DEBUG_PRINTLN("OK: "+String(config.stream_url));
    return true;
  } else {
    return false;
  }
}
