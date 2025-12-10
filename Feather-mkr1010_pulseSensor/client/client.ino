#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>
#include "arduino_secrets.h"
#include "LEDAnimationFunctions.h"

const char* ssid     = SECRET_SSID;
const char* password = SECRET_PASS;
const char* mqtt_server   = "mqtt.cetools.org";
int mqtt_port = 1883;
const char* mqtt_topic = "student/ucfnake/pulse";      

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

int status = WL_IDLE_STATUS;

void setup() {
  Serial.begin(9600);
  delay(200);

  Serial.println("Attempting to connect to WiFi...");

  status = WiFi.begin(ssid, password);
  if (status != WL_CONNECTED) {
    Serial.println("Unable to connect to network");
    while (true);
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Connecting to MQTT broker...");

  if (!mqttClient.connect(mqtt_server, mqtt_port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    while (1);
  }
  Serial.println("Connected to MQTT.");


  mqttClient.subscribe(mqtt_topic);
  Serial.print("Subscribed to topic: ");
  Serial.println(mqtt_topic);

  initStressDisplay();
}

void loop() {
 
  mqttClient.poll();

  // If there is a message available
  if (mqttClient.parseMessage()) {
    while (mqttClient.available()) {
      char c = mqttClient.read();

      if (c >= '0' && c <= '9') {
        int value = c - '0';
        setStressLevel(value);  
        updateStressDisplay();
      }
    }
  }
}
