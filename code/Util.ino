String uptime() {
  return calculate_time(millis());
}

String calculate_time(unsigned int mil) {
  long days = 0;
  long hours = 0;
  long mins = 0;
  long secs = 0;
  String output = "";

  secs = mil / 1000;
  mins = secs / 60;
  hours = mins / 60;
  days = hours / 24;
  secs = secs - (mins * 60);
  mins = mins - (hours * 60);
  hours = hours - (days * 24);
  if (days > 0) {
    output += days;
    if (days == 1) {
      output += " day ";
    } else {
      output += " days ";
    }
  }
  output += hours < 10 ? "0" : "";
  output += hours;
  output += ":";
  output += mins < 10 ? "0" : "";
  output += mins;
  output += ":";
  output += secs < 10 ? "0" : "";
  output += secs;

  return output;
}

String generateJSONData() {
  String output;
  json_web_data.clear();

  for (JsonPair kv : json_obis_data.as<JsonObject>()) {
    if (!kv.value()["value"]) kv.value()["value"] = 0;

    String obis_name = kv.value()["name"].as<String>();
    String obis_unit = kv.value()["unit"].as<String>();
    String obis_value = kv.value()["value"].as<String>();
    boolean obis_publish = kv.value()["publish"].as<boolean>();

    //    obis_name += " - ";
    //    obis_name += kv.key().c_str();

    if (obis_publish) {
      String history = "";
      for (int i = 0; i < DATA_HISTORY_SIZE; i++) {
        history += String(kv.value()["history" + String(i)].as<float>());
        if (i < DATA_HISTORY_SIZE)history += ",";
      }

      json_web_data[obis_name]["value"] = obis_value;
      json_web_data[obis_name]["unit"] = obis_unit;
      json_web_data[obis_name]["history"] = history;
    }
  }

  if (last_invalid_crc_millis > 0) {
    json_web_data["Last CRC error"]["value"] = calculate_time(last_invalid_crc_millis);
    json_web_data["Last CRC error"]["unit"] = String(invalid_crc_count) + " / " + String(total_data_count) + " (" + String((float)(100.0 / (float)total_data_count) * (float)invalid_crc_count) + "%)";
  }

  serializeJson(json_web_data, output);

  return output;
}

String getSplitValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void hard_restart() {
  esp_task_wdt_init(1,true);
  esp_task_wdt_add(NULL);
  while(true);
}
