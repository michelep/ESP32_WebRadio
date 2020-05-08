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
    return String();
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
    String message;
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
        response = String("{\"status\":200}");
      } else if(action.equals("set_contrast")) {
        config.contrast = atoi(request->getParam("value", true)->value().c_str());       
        response = String("{\"status\":200}");
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
