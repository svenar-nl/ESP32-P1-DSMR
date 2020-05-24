bool decodeTelegram(int len) {
  //need to check for start
  int startChar = FindCharInArrayRev(telegram, '/', len);
  int endChar = FindCharInArrayRev(telegram, '!', len);
  bool endOfMessage = false;
  if (startChar >= 0) {
    if (config_debug) {
      for (int cnt = startChar; cnt < len - startChar; cnt++)
        Serial.print(telegram[cnt]);
    }
  } else if (endChar >= 0) {
    endOfMessage = true;
    if (config_debug) {
      for (int cnt = 0; cnt < len; cnt++)
        Serial.print(telegram[cnt]);
    }
  } else {
    if (config_debug) {
      for (int cnt = 0; cnt < len; cnt++)
        Serial.print(telegram[cnt]);
    }
  }

  for (JsonPair kv : json_obis_data.as<JsonObject>()) {
    String obis = kv.key().c_str();
    if (strncmp(telegram, obis.c_str(), strlen(obis.c_str())) == 0) {
      boolean obis_publish = kv.value()["publish"].as<boolean>();
      boolean obis_mqtt_topic = kv.value()["mqtt_topic"].as<boolean>();
      String obis_value = String(getValue(telegram, len));
      kv.value()["value"] = obis_value;
      
      if (obis_publish) {
        mqtt_publish(config_mqtt_topic_base + String("/") + obis_mqtt_topic, obis_value);
      }
    }
  }

  //
  //  long val = 0;
  //  long val2 = 0;
  //  // 1-0:1.8.1(000992.992*kWh)
  //  // 1-0:1.8.1 = Elektra verbruik laag tarief (DSMR v4.0)
  //  if (strncmp(telegram, "1-0:1.8.1", strlen("1-0:1.8.1")) == 0)
  //    powerConsumptionLowTariff = getValue(telegram, len);
  //
  //  // 1-0:1.8.2(000560.157*kWh)
  //  // 1-0:1.8.2 = Elektra verbruik hoog tarief (DSMR v4.0)
  //  if (strncmp(telegram, "1-0:1.8.2", strlen("1-0:1.8.2")) == 0)
  //    powerConsumptionHighTariff = getValue(telegram, len);
  //
  //  // 1-0:2.8.1(000348.890*kWh)
  //  // 1-0:2.8.1 = Elektra opbrengst laag tarief (DSMR v4.0)
  //  if (strncmp(telegram, "1-0:2.8.1", strlen("1-0:2.8.1")) == 0)
  //    powerProductionLowTariff = getValue(telegram, len);
  //
  //  // 1-0:2.8.2(000859.885*kWh)
  //  // 1-0:2.8.2 = Elektra opbrengst hoog tarief (DSMR v4.0)
  //  if (strncmp(telegram, "1-0:2.8.2", strlen("1-0:2.8.2")) == 0)
  //    powerProductionHighTariff = getValue(telegram, len);
  //
  //  // 1-0:1.7.0(00.424*kW) Actueel verbruik
  //  // 1-0:2.7.0(00.000*kW) Actuele teruglevering
  //  // 1-0:1.7.x = Electricity consumption actual usage (DSMR v4.0)
  //  if (strncmp(telegram, "1-0:1.7.0", strlen("1-0:1.7.0")) == 0)
  //    CurrentPowerConsumption = getValue(telegram, len);
  //
  //  if (strncmp(telegram, "1-0:2.7.0", strlen("1-0:2.7.0")) == 0)
  //    CurrentPowerProduction = getValue(telegram, len);
  //
  //  // 0-1:24.2.1(150531200000S)(00811.923*m3)
  //  // 0-1:24.2.1 = Gas (DSMR v4.0) on Kaifa MA105 meter
  //  if (strncmp(telegram, "0-1:24.2.1", strlen("0-1:24.2.1")) == 0)
  //    GasConsumption = getValue(telegram, len);

  return endOfMessage;
}

long getValue(char *buffer, int maxlen) {
  int s = FindCharInArrayRev(buffer, '(', maxlen - 2);
  if (s < 8)
    return 0;
  if (s > 32)
    s = 32;
  int l = FindCharInArrayRev(buffer, '*', maxlen - 2) - s - 1;
  if (l < 4)
    return 0;
  if (l > 12)
    return 0;
  char res[16];
  memset(res, 0, sizeof(res));

  if (strncpy(res, buffer + s + 1, l)) {
    if (isNumber(res, l)) {
      return (1000 * atof(res));
    }
  }
  return 0;
}

int FindCharInArrayRev(char array[], char c, int len) {
  for (int i = len - 1; i >= 0; i--) {
    if (array[i] == c) {
      return i;
    }
  }
  return -1;
}

bool isNumber(char *res, int len) {
  for (int i = 0; i < len; i++) {
    if (((res[i] < '0') || (res[i] > '9')) && (res[i] != '.' && res[i] != 0)) {
      return false;
    }
  }
  return true;
}
