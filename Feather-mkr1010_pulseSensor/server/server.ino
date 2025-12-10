#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <math.h>
#include "secrets.h"

// --- MQTT & WIFI CONFIGURATION ---
const char* ssid          = SECRET_SSID;
const char* password      = SECRET_PASS;
const char* mqtt_username = SECRET_MQTTUSER;
const char* mqtt_password = SECRET_MQTTPASS;

// Broker configuration 
const char* mqtt_server   = "mqtt.cetools.org"; 
const int mqtt_port       = 1884;               
const char* MQTT_CLIENT_ID = "";   
const char* MQTT_TOPIC_STATE = "student/ucfnake/pulse";      

WiFiClient espClient;
PubSubClient client(espClient);

// PULSE SENSOR SETTINGS 
const int SENSOR_PIN = 14;      
const int THRESHOLD  = 520;     

bool beatDetected = false;
unsigned long lastBeatTime = 0;
unsigned long IBI = 0;

// RMSSD / STRESS VARIABLES 
const int MAX_IBI_SAMPLES = 10;
unsigned long ibiList[MAX_IBI_SAMPLES];
int ibiIndex = 0;
bool ibiBufferFilled = false;

int stressState = 0;  

const unsigned long NO_SIGNAL_TIMEOUT = 3000;


// ========== WIFI + MQTT SETUP ==========
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT... ");
    String clientId = "ESP8266-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected!");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 2 seconds...");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
}


// ========== MAIN LOOP ==========
void loop() {

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  readPulseSensor();
  checkNoSignal();
  publishStressState();

  delay(5);
}


// ========== PULSE READING ==========
void readPulseSensor() {
  int rawValue = analogRead(SENSOR_PIN);
  unsigned long currentTime = millis();

  // Beat detection
  if (rawValue > THRESHOLD && !beatDetected) {
    beatDetected = true;

    if (lastBeatTime > 0) {
      IBI = currentTime - lastBeatTime;
      storeIBI(IBI);
      computeStress();
    }

    lastBeatTime = currentTime;
  }

  if (rawValue < THRESHOLD - 50) {
    beatDetected = false;
  }
}


void checkNoSignal() {
  unsigned long currentTime = millis();

  if (lastBeatTime > 0 && (currentTime - lastBeatTime) > NO_SIGNAL_TIMEOUT) {
    stressState = 0;

    ibiIndex = 0;
    ibiBufferFilled = false;
  }
}


void storeIBI(unsigned long ibi) {
  ibiList[ibiIndex] = ibi;
  ibiIndex++;

  if (ibiIndex >= MAX_IBI_SAMPLES) {
    ibiIndex = 0;
    ibiBufferFilled = true;
  }
}


void computeStress() {
  if (!ibiBufferFilled) return;

  double sumSqDiff = 0;
  for (int i = 0; i < MAX_IBI_SAMPLES - 1; i++) {
    float diff = (float)ibiList[i + 1] - (float)ibiList[i];
    sumSqDiff += diff * diff;
  }

  float rmssd = sqrt(sumSqDiff / (MAX_IBI_SAMPLES - 1));

  if (rmssd > 60)      stressState = 1;
  else if (rmssd > 40) stressState = 2;
  else if (rmssd > 20) stressState = 3;
  else                 stressState = 4;
}


void publishStressState() {
  static unsigned long lastSend = 0;

  if (millis() - lastSend >= 300) {    // send ~3× per second
    lastSend = millis();

    char payload[8];
    sprintf(payload, "%d", stressState);
    client.publish(MQTT_TOPIC, payload);
  }
}
