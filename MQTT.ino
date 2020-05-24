void setup_mqtt() {
  mqtt_client.setServer(config_mqtt_host.c_str(), config_mqtt_port);
  mqtt_client.setCallback(mqtt_callback);

  if (mqtt_client.connect("p1smartmeterclient", config_mqtt_username.c_str(), config_mqtt_password.c_str())) {
    // mqtt_client.publish((config_mqtt_topic_base + String("/") + String("p1smartmeter")).c_str(), "Hello, World!");
  }
}

void mqtt_publish(String topic, String msg) {
  if (config_mqtt_enabled) mqtt_client.publish((config_mqtt_topic_base + String("/") + topic).c_str(), msg.c_str());
}

void mqtt_callback(char* topic, byte * message, unsigned int length) {

}
