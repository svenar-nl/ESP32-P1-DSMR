void decodeData(String data_in, int len) {
  String buffer = "";
  String crc_buffer = "";
  String data_crc = "";
  int crc_buffer_length = 0;
  boolean append_crc_data = true;

  for (unsigned int i = 0; i < len + 1; i++) {
    char c = data_in[i];
    if (append_crc_data) {
      crc_buffer += c;
      crc_buffer_length += 1;
    } else data_crc += c;

    if (c == '!') append_crc_data = false;
  }

  unsigned int data_crc_parsed = strtol(data_crc.c_str(), 0, 16);
  unsigned int crc_calc = p1_crc16(crc_buffer);
  boolean valid_crc = crc_calc == data_crc_parsed;

  if (!valid_crc) {
    last_invalid_crc_millis = millis();
    invalid_crc_count++;
  }
  total_data_count++;

  if (!config_require_crc) valid_crc = true;

  for (unsigned int i = 0; i < len + 1; i++) {
    char c = data_in[i];
    buffer += c;

    if (c == '\n') {
      decodeLine(buffer, buffer.length(), valid_crc);
      buffer = "";
    }
  }
}

void decodeLine(String line, int len, boolean valid_crc) {
  String obis_ref = "";
  float parsed_value = 0.0;
  String tmp_value = "";
  String tmp_parsed_value = "";

  if (line.indexOf("(") > 0) {
    line.replace(")", "");
    line.replace("m3", ""); // Sneaky bastard

    obis_ref = getSplitValue(line, '(', 0);


    if (json_obis_data.as<JsonObject>()[obis_ref]) {
      JsonObject tmp_json_obis_data = json_obis_data.as<JsonObject>()[obis_ref].as<JsonObject>();

      int _value_field = tmp_json_obis_data["value_field"].as<int>();
      if (_value_field == 0) _value_field = 1;

      tmp_parsed_value = getSplitValue(line, '(', _value_field);
      for (unsigned int i = 0; i < tmp_parsed_value.length() + 1; i++) {
        char c = tmp_parsed_value[i];

        if (isDigit(c) || c == '.') {
          tmp_value += c;
        }
      }

      if (tmp_value.length() > 0) {
        parsed_value = tmp_value.toFloat();

        String _publish = tmp_json_obis_data["publish"].as<String>();
        String _mqtt_topic = tmp_json_obis_data["mqtt_topic"].as<String>();
        int _count = tmp_json_obis_data["count"].as<int>();
        int _average = tmp_json_obis_data["average"].as<int>();
        int _multiplier = tmp_json_obis_data["multiplier"].as<int>();

        if (_publish.equalsIgnoreCase("true")) {
          if (_multiplier > 0) parsed_value *= _multiplier;

          if (_count > 0 || _average > 0) {
            if (_count > 0) {
              json_obis_data.as<JsonObject>()[obis_ref]["tmp_count"] = json_obis_data.as<JsonObject>()[obis_ref]["tmp_count"].as<int>() + 1;
              // json_obis_data.as<JsonObject>()[obis_ref]["tmp_value"] = json_obis_data.as<JsonObject>()[obis_ref]["tmp_value"].as<float>() + parsed_value;
              if (valid_crc) {
                json_obis_data.as<JsonObject>()[obis_ref]["last_valid_value"] = parsed_value;
              }
              if (json_obis_data.as<JsonObject>()[obis_ref]["tmp_count"].as<int>() >= _count) {
                json_obis_data.as<JsonObject>()[obis_ref]["value"] = json_obis_data.as<JsonObject>()[obis_ref]["last_valid_value"].as<float>();
                // json_obis_data.as<JsonObject>()[obis_ref]["value"] = json_obis_data.as<JsonObject>()[obis_ref]["tmp_value"].as<float>() / json_obis_data.as<JsonObject>()[obis_ref]["tmp_count"].as<float>();
                // json_obis_data.as<JsonObject>()[obis_ref]["tmp_value"] = 0;
                json_obis_data.as<JsonObject>()[obis_ref]["tmp_count"] = 0;

                if (json_obis_data.as<JsonObject>()[obis_ref]["value"].as<float>() > 0) {
                  json_obis_data.as<JsonObject>()[obis_ref]["startup_done"] = true;
                }

                if (json_obis_data.as<JsonObject>()[obis_ref]["startup_done"].as<boolean>()) {
                  //                  for (int i = 0; i < DATA_HISTORY_SIZE; i++) {
                  //                    if (i < DATA_HISTORY_SIZE) {
                  //                      json_obis_data.as<JsonObject>()[obis_ref]["history" + String(i)] = json_obis_data.as<JsonObject>()[obis_ref]["history" + String(i + 1)].as<float>();
                  //                    } else {
                  //                      json_obis_data.as<JsonObject>()[obis_ref]["history" + String(i)] = json_obis_data.as<JsonObject>()[obis_ref]["value"].as<float>();
                  //                    }
                  //                  }
                  for (int i = 0; i < DATA_HISTORY_SIZE; i++) {
                    if (i + 1 <= DATA_HISTORY_SIZE) {
                      json_obis_data.as<JsonObject>()[obis_ref]["history" + String(i)] = json_obis_data.as<JsonObject>()[obis_ref]["history" + String(i + 1)].as<float>();
                    }

                  }
                  json_obis_data.as<JsonObject>()[obis_ref]["history" + String(DATA_HISTORY_SIZE)] = json_obis_data.as<JsonObject>()[obis_ref]["value"].as<float>();

                  mqtt_publish(_mqtt_topic, json_obis_data.as<JsonObject>()[obis_ref]["value"].as<String>());
                }
              }
            }

            if (_average > 0) {
              json_obis_data.as<JsonObject>()[obis_ref]["tmp_average"] = json_obis_data.as<JsonObject>()[obis_ref]["tmp_average"].as<int>() + 1;
              if (valid_crc) {
                json_obis_data.as<JsonObject>()[obis_ref]["tmp_value"] = json_obis_data.as<JsonObject>()[obis_ref]["tmp_value"].as<float>() + parsed_value;
                json_obis_data.as<JsonObject>()[obis_ref]["last_valid_value"] = parsed_value;
              } else {
                json_obis_data.as<JsonObject>()[obis_ref]["tmp_value"] = json_obis_data.as<JsonObject>()[obis_ref]["tmp_value"].as<float>() + json_obis_data.as<JsonObject>()[obis_ref]["last_valid_value"].as<float>();
              }

              if (json_obis_data.as<JsonObject>()[obis_ref]["tmp_average"].as<int>() >= _average) {
                json_obis_data.as<JsonObject>()[obis_ref]["value"] = json_obis_data.as<JsonObject>()[obis_ref]["tmp_value"].as<float>() / json_obis_data.as<JsonObject>()[obis_ref]["tmp_average"].as<float>();
                json_obis_data.as<JsonObject>()[obis_ref]["tmp_value"] = 0;
                json_obis_data.as<JsonObject>()[obis_ref]["tmp_average"] = 0;

                if (json_obis_data.as<JsonObject>()[obis_ref]["value"].as<float>() > 0) {
                  json_obis_data.as<JsonObject>()[obis_ref]["startup_done"] = true;
                }

                if (json_obis_data.as<JsonObject>()[obis_ref]["startup_done"].as<boolean>()) {
                  //                  for (int i = 0; i < DATA_HISTORY_SIZE; i++) {
                  //                    if (i < DATA_HISTORY_SIZE) {
                  //                      json_obis_data.as<JsonObject>()[obis_ref]["history" + String(i)] = json_obis_data.as<JsonObject>()[obis_ref]["history" + String(i + 1)].as<float>();
                  //                    } else {
                  //                      json_obis_data.as<JsonObject>()[obis_ref]["history" + String(i)] = json_obis_data.as<JsonObject>()[obis_ref]["value"].as<float>();
                  //                    }
                  //                  }
                  for (int i = 0; i < DATA_HISTORY_SIZE; i++) {
                    if (i + 1 <= DATA_HISTORY_SIZE) {
                      json_obis_data.as<JsonObject>()[obis_ref]["history" + String(i)] = json_obis_data.as<JsonObject>()[obis_ref]["history" + String(i + 1)].as<float>();
                    }

                  }
                  json_obis_data.as<JsonObject>()[obis_ref]["history" + String(DATA_HISTORY_SIZE)] = json_obis_data.as<JsonObject>()[obis_ref]["value"].as<float>();

                  mqtt_publish(_mqtt_topic, json_obis_data.as<JsonObject>()[obis_ref]["value"].as<String>());
                }
              }
            }
          } else {
            json_obis_data.as<JsonObject>()[obis_ref]["value"] = parsed_value;

            //            for (int i = 0; i < DATA_HISTORY_SIZE; i++) {
            //              if (i < DATA_HISTORY_SIZE) {
            //                json_obis_data.as<JsonObject>()[obis_ref]["history" + String(i)] = json_obis_data.as<JsonObject>()[obis_ref]["history" + String(i + 1)].as<float>();
            //              } else {
            //                json_obis_data.as<JsonObject>()[obis_ref]["history" + String(i)] = json_obis_data.as<JsonObject>()[obis_ref]["value"].as<float>();
            //              }
            //            }

            for (int i = 0; i < DATA_HISTORY_SIZE; i++) {
              if (i + 1 <= DATA_HISTORY_SIZE) {
                json_obis_data.as<JsonObject>()[obis_ref]["history" + String(i)] = json_obis_data.as<JsonObject>()[obis_ref]["history" + String(i + 1)].as<float>();
              }

            }
            json_obis_data.as<JsonObject>()[obis_ref]["history" + String(DATA_HISTORY_SIZE)] = json_obis_data.as<JsonObject>()[obis_ref]["value"].as<float>();

            mqtt_publish(_mqtt_topic, json_obis_data.as<JsonObject>()[obis_ref]["value"].as<String>());
          }

        }
      }

    }
  }
}
