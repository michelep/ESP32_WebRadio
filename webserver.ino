// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty

String templateProcessor(const String& var)
{
  //
  // System values
  //
  if(var == "hostname") {
    return String(config.hostname);
  }
  if(var == "fw_name") {
    return String(FW_NAME);
  }
  if(var=="fw_version") {
    return String(FW_VERSION);
  }
  if(var=="uptime") {
    return String(millis()/1000);
  }
  if(var=="timedate") {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      DEBUG_PRINTLN("Failed to obtain time");
      return String("Error");
    }
    return String(String(timeinfo.tm_hour)+":"+String(timeinfo.tm_min)+":"+String(timeinfo.tm_sec));
  }
  if(var == "ota_enable") {
    if(config.ota_enable) {
      return String("checked");
    } else {
      return "";
    }
  }
  
  //
  // Config values
  //
  if(var=="wifi_essid") {
    return String(config.wifi_essid);
  }
  if(var=="wifi_password") {
    return String(config.wifi_password);
  }
  if(var=="wifi_rssi") {
    return String(WiFi.RSSI());
  }
  if(var=="ntp_server") {
    return String(config.ntp_server);
  }
  if(var=="ntp_timezone") {
    return String(config.ntp_timezone);
  }  
  //
  if(var=="stream_url") {
    return String(config.stream_url);
  }
  if(var=="stream_id") {
    return String(config.stream_id);
  }
  if(var=="volume") {
    return String(config.volume);
  }
  if(var=="contrast") {
    return String(config.contrast);
  }
  //
  return String();
}
// ************************************
// initWebServer
//
// initialize web server
// ************************************
void initWebServer() {
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html").setTemplateProcessor(templateProcessor);

  server.on("/restart", HTTP_POST, [](AsyncWebServerRequest *request) {
    ESP.restart();
  });
  
  server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request) {
    String action;
    if(request->hasParam("action", true)) {
      action = request->getParam("action", true)->value();
      if(action.equals("config")) {
        if(request->hasParam("wifi_essid", true)) {
          strcpy(config.wifi_essid,request->getParam("wifi_essid", true)->value().c_str());
        }
        if(request->hasParam("wifi_password", true)) {
          strcpy(config.wifi_password,request->getParam("wifi_password", true)->value().c_str());
        }
        if(request->hasParam("ntp_server", true)) {
          strcpy(config.ntp_server, request->getParam("ntp_server", true)->value().c_str());
        }
        if(request->hasParam("ntp_timezone", true)) {
          config.ntp_timezone = atoi(request->getParam("ntp_timezone", true)->value().c_str());
        }
        if(request->hasParam("stream_url", true)) {
          strcpy(config.stream_url, request->getParam("stream_url", true)->value().c_str());
          streamChanged=true;
        }
        saveConfigFile();    
      } else if(action.equals("streams")) {
        DynamicJsonDocument doc(512);
        JsonArray streams = doc["streams"].to<JsonArray>();
        // POST variable stream[] contains all streams
        // {"action":"streams","stream[]":["http://stream.dancewave.online:8080/dance.mp3","http://mxsel.maksmedia.ru:8000/maksfm128","https://icecast.unitedradio.it/Radio105.mp3",""]}
        int paramsNr = request->params();
        // Iterate over all POST params...
        for(int i=0;i<paramsNr;i++){
          AsyncWebParameter* p = request->getParam(i);
          if(p->name().equals("stream[]")) {
            if(p->value().length() > 0) {
              streams.add(p->value());  
            }
          }
        }
        // Save JSON to streams.json
        File dbFile = SPIFFS.open(DB_FILE, "w");
        if(!dbFile) {
          DEBUG_PRINTLN("ERROR: Failed to create db file !");
          return false;
        }
        serializeJson(doc,dbFile);
        dbFile.close();
      }
    }
    request->redirect("/?result=ok");
  });
  
  server.on("/ajax", HTTP_POST, [] (AsyncWebServerRequest *request) {
    String action,value,response="";
    char outputJson[256];

    if(request->hasParam("action", true)) {
      action = request->getParam("action", true)->value();
      if(action.equals("get")) {
        value = request->getParam("value", true)->value();
        if(value.equals("env")) {
          serializeJson(env,outputJson);
          response = String(outputJson);
        }
      } else if(action.equals("set_volume")) {
        config.volume = atoi(request->getParam("value", true)->value().c_str());       
        response = String("{\"status\":200, \"volume\": "+String(config.volume)+"}");
      } else if(action.equals("set_contrast")) {
        config.contrast = atoi(request->getParam("value", true)->value().c_str());       
        response = String("{\"status\":200}");
      } else if(action.equals("toggle_play")) {
        togglePlay();
        response = String("{\"status\":200, \"status\": "+String(streamIsPlaying)+"}");
      } else if(action.equals("play_stream")) {
        config.stream_id=atoi(request->getParam("value", true)->value().c_str()) | 0;
        streamChanged=true;
        response = String("{\"status\":200, \"stream_id\": "+String(config.stream_id)+"}");
      } else if(action.equals("next_stream")) {
        nextStream(); 
        response = String("{\"status\":200, \"stream_id\": "+String(config.stream_id)+"}");
      }
    }
    request->send(200, "text/plain", response);
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.printf("[404] Not Found: http://%s%s</p>", request->host().c_str(), request->url().c_str());
   
    request->send(404);
  });

  server.begin();
}
