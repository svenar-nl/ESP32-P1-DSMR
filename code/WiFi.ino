void setupWifi() {
  WiFiManager wm;
  wm.setClass("invert"); // dark theme
  //  wm.setScanDispPerc(true); // display percentages instead of graphs for RSSI
  wm.setDebugOutput(false);
  wm.setAPCallback(configModeCallback);
  bool wmresult = wm.autoConnect(DEFAULT_WIFIMANAGER_AP_NAME, DEFAULT_WIFIMANAGER_PASSWORD);

  //  WiFi.disconnect(true);

//  wm.resetSettings();

  if (!wmresult) {
    //ESP.restart();
    hard_restart();
    delay(1000);
    //    if (wifi_reconnect_count < 5) {
    //      wifi_reconnect_count++;
    //      Serial.println("Failed to connect");
    //      delay(5000);
    //      setupWifi();
    //    } else {
    //      ESP.restart();
    //    }
  } //else {
  //    wifi_reconnect_count = 0;
  //  }
}

void configModeCallback (WiFiManager *myWiFiManager) {
  ticker.attach(0.2, tick);
}

void setupWebaddresses() {
  // Website Data
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html");
  });

  server.on("/config", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/config.html");
  });

  server.on("/about", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/about.html");
  });

  server.on("/config.json", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/config.json");
  });

  // Website Requirements
  server.on("/pure-min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/pure-min.css");
  });
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/style.css");
  });
  server.on("/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/jquery.min.js");
  });
  server.on("/ui.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/ui.js");
  });

  // Web Utils
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest * request) {
    String output = generateJSONData();

    request->send(200, "application/json", output);
  });

  server.on("/uptime", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", uptime().c_str());
  });

  server.on("/ip", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", WiFi.localIP().toString().c_str());
  });

  server.on("/mac", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", WiFi.macAddress().c_str());
  });

  server.on("/version", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", String(FW_VERSION).c_str());
  });

  server.on("/meminfo", HTTP_GET, [](AsyncWebServerRequest * request) {
    String mem_data = "";
    mem_data += esp_get_free_heap_size() / 1024;
    mem_data += "MB free heap / ";
    mem_data += esp_get_minimum_free_heap_size() / 1024;
    mem_data += "MB minimum free heap";
    request->send_P(200, "text/plain", mem_data.c_str());
  });

  server.on("/saveconfig", HTTP_POST, [](AsyncWebServerRequest * request) {
    String config_data;
    if (request->hasParam("config", true)) {
      config_data = request->getParam("config", true)->value();

      File file = SPIFFS.open("/config.json", FILE_WRITE);

      if (!file) {
        request->send(200, "text/plain", "ERROR:can't open config.json");
        return;
      }

      if (!file.print(config_data)) {
        request->send(200, "text/plain", "ERROR:can't write to config.json");
        return;
      }

      file.close();

      request->send(200, "text/plain", "OK");
      ESP.restart();
    } else {
      request->send(200, "text/plain", "ERROR:no config data received");
    }
  });

  // SETTINGS

  server.on("/wipewifisettings", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", "1");
    WiFi.disconnect(false, true);
    ESP.restart();
  });

  server.on("/factoryreset", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", "1");
    WiFi.disconnect(false, true);
    // TODO: Reset config.json
    ESP.restart();
  });

  server.onNotFound(notFound);
}

void notFound(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/404.html");
}
