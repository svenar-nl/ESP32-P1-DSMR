/*
  UART    RX IO   TX IO   CTS     RTS
  UART0   GPIO3   GPIO1   N/A     N/A
  UART1   GPIO9   GPIO10  GPIO6   GPIO11
  UART2   GPIO16  GPIO17  GPIO8   GPIO7
*/

/*
   CHANGELOG

   V1.3
   [code] Fixed restart sometimes sends 0 as value.

   V1.4
   [web] Added PubSubClient to license in about.html
   [web] Added ArduinoJSON to license in about.html

   [config] Added WiFi hostname
   [config] Changed obis: [] to data: []
   🔥🔥
   [code] WiFi hostname set from the value in the config.json file
   [code] Fixed uptime 1 day displayed as 1 days
   [code] Cleaned up unused variables

   V1.5
   [code] Added CRC validation

   V1.6
   [web] removed /debug
   [web] removed /crc

   [code] Removed debug stuff
   [code] Fixed CRC validation
   [code] P1 telegram parsing is now done outside of the while loop

   V1.7
   [code] Added history to p1 data
   [code] Added field 'history' to json web data
   [code] Added Reboot after 5 failed WiFi connect attempts
   [code] Changed text/pain to application/json in wifi /data request

   V1.8
   [code] Changed AsyncWifiManager to WifiManager
   [code] Changed ESP.restart(); to a more hard reset
   [code] Device uptime in seconds is posted on MQTT with 10 seconds interval
   [code] Removed config debug data
   
   [config] Removed debug option from config.json
*/

#include <WiFi.h>
#include <SPIFFS.h>
#include <WiFiManager.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <esp_int_wdt.h>
#include <esp_task_wdt.h>

#define FW_VERSION                    "V1.8 (" + String( __DATE__) + ")"
#define DEFAULT_OTA_NAME              "p1-smartmeter-ota"
#define DEFAULT_WIFIMANAGER_AP_NAME   "P1 SmartMeter"
#define DEFAULT_WIFIMANAGER_PASSWORD  "Welcome01"
#define P1_SERIAL                     Serial2
#define LED_CONNECTED                 19
#define LED_DATA                      18
#define DATA_HISTORY_SIZE             10
#define UPTIME_POST_TIME_MS           10 * 1000
#define MQTT_UPTIME_TOPIC             "uptime"

AsyncWebServer server(80);
WiFiClient espClient;
PubSubClient mqtt_client(espClient);
StaticJsonDocument<16384> json_obis_data;
StaticJsonDocument<8192> json_web_data;
Ticker ticker;

bool /*config_debug, */config_ota_enabled, config_mqtt_enabled, config_require_crc;
String config_mqtt_username, config_mqtt_password, config_mqtt_host, config_mqtt_topic_base, config_wifi_hostname;
unsigned int config_mqtt_port, last_invalid_crc_millis, invalid_crc_count, total_data_count;
long config_p1_serial_baud;
unsigned long uptime_post_millis = 0;

boolean message_complete = false;
unsigned int data_in_length = 0;
String rxbuf = "";
int config_num_obis_refs = 0;
int wifi_reconnect_count = 0;

void setup() {
  WiFi.mode(WIFI_STA);
  
  Serial.begin(115200);
  Serial.setDebugOutput(0);

  pinMode(LED_CONNECTED, OUTPUT);
  pinMode(LED_DATA, OUTPUT);

  ticker.attach(0.6, tick);

  setupWifi();

  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  load_config();
  P1_SERIAL.begin(config_p1_serial_baud);

  WiFi.setHostname(config_wifi_hostname.c_str());

//  if (config_debug) {
//    Serial.println("-----SETTINGS-----");
//    Serial.print("IP: "); Serial.println(WiFi.localIP());
//    Serial.print("DEBUG: "); Serial.println(config_debug ? "true" : "false");
//    Serial.print("OTA: "); Serial.println(config_ota_enabled ? "true" : "false");
//    Serial.print("P1 BAUD: "); Serial.println(config_p1_serial_baud);
//    Serial.println("------------------");
//  }

  //  setupWebsocketServer();
  setupWebaddresses();
  server.begin();

  if (config_mqtt_enabled) setup_mqtt();

  if (config_ota_enabled) setup_ota();

  ticker.detach();
  digitalWrite(LED_CONNECTED, HIGH);
}

void loop() {
  if (config_ota_enabled) ArduinoOTA.handle();
  if (!mqtt_client.connected()) {
    setup_mqtt();
  }
  mqtt_client.loop();

  p1_serial_listener();
  p1_data_listener();

  if (millis() - uptime_post_millis > UPTIME_POST_TIME_MS) {
    uptime_post_millis = millis();
    mqtt_publish(MQTT_UPTIME_TOPIC, String(millis() / 1000));
  }
}

void p1_serial_listener() {
  if (P1_SERIAL.available()) {
    while (P1_SERIAL.available()) {
      int c = P1_SERIAL.read();
      if (c < 0) {
        message_complete = false;
        data_in_length = 0;
        rxbuf = "";
        break;
      }
      data_in_length++;
      if (c == '/') {
        message_complete = false;
        data_in_length = 0;
        rxbuf = "";
      }
      if (c == '!') {
        message_complete = true;
      }
      rxbuf.concat((char)c);
    }
  }
}

void p1_data_listener() {
  //  if (P1_SERIAL.available()) {
  //    digitalWrite(LED_DATA, HIGH);

  if (message_complete) {
    digitalWrite(LED_DATA, HIGH);
    message_complete = false;
    decodeData(rxbuf, data_in_length);
    data_in_length = 0;
    rxbuf = "";
    digitalWrite(LED_DATA, LOW);
  }
}

void tick() {
  int state = digitalRead(LED_CONNECTED);
  digitalWrite(LED_CONNECTED, !state);
}
