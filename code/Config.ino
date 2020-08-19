void load_config() {
  File file = SPIFFS.open("/config.json", FILE_READ);

  if (!file) {
    Serial.println("There was an error opening the file for reading");
    return;
  }

  String config_data = "";

  while (file.available()) {
    config_data += char(file.read());
  }

  DynamicJsonDocument doc(16384);
  deserializeJson(doc, config_data);
  JsonObject obj = doc.as<JsonObject>();

  //boolean debug = obj["debug"].as<boolean>();
  boolean ota_enabled = obj["ota_updates"].as<boolean>();
  boolean require_crc = obj["require_crc"].as<boolean>();
  boolean mqtt_enabled = obj["mqtt"]["enabled"].as<boolean>();
  
  String mqtt_username = obj["mqtt"]["username"].as<String>();
  String mqtt_password = obj["mqtt"]["password"].as<String>();
  String mqtt_host = obj["mqtt"]["host"].as<String>();
  unsigned int mqtt_port = obj["mqtt"]["port"].as<int>();
  String mqtt_topic_base = obj["mqtt"]["topic_base"].as<String>();

  long p1_serial_baud = obj["serial_baudrate"].as<long>();

  //config_debug = debug;
  config_ota_enabled = ota_enabled;
  config_require_crc = require_crc;
  config_mqtt_enabled = mqtt_enabled;
  config_mqtt_username = mqtt_username;
  config_mqtt_password = mqtt_password;
  config_mqtt_host = mqtt_host;
  config_mqtt_port = mqtt_port;
  config_mqtt_topic_base = mqtt_topic_base;
  config_p1_serial_baud = p1_serial_baud;

  //  for (JsonPair kv : obj["obis"].as<JsonObject>()) {
  //    json_obis_data[kv.key().c_str()] = kv.value();
  //    Serial.print(kv.key().c_str());
  //    Serial.print(" - ");
  //    Serial.println(kv.value().as<String>());
  //  }

  for (JsonVariant v : obj["data"].as<JsonArray>()) {
    json_obis_data[v.as<JsonObject>()["obis"].as<String>()] = v.as<JsonObject>();
    //        Serial.print(v.as<JsonObject>()["obis"].as<String>());
    //        Serial.print(" - ");
    //        Serial.println(v.as<String>());
    //    Serial.println(v.as<String>());
  }

  file.close();
}
