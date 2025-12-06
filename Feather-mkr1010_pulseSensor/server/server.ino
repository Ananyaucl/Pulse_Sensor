#include <ESP8266WiFi.h>
#include <math.h>
#include "secrets.h"

// WIFI SETTINGS 
// const char* ssid     = "CE-Hub-Student";
// const char* password = "casa-ce-gagarin-public-service";

const char* ssid     = SECRET_SSID;
const char* password = SECRET_PASS;

WiFiServer server(5000);
WiFiClient client;

// PULSE SENSOR SETTINGS 
const int SENSOR_PIN = 14;      // Only ADC on ESP8266
const int THRESHOLD  = 550;     // Adjust depending on your sensor

// Beat detection helper variables
bool beatDetected = false;
unsigned long lastBeatTime = 0;
unsigned long IBI = 0;          // Inter-Beat Interval (ms)

// RMSSD / STRESS VARIABLES 
const int MAX_IBI_SAMPLES = 10;
unsigned long ibiList[MAX_IBI_SAMPLES];
int ibiIndex = 0;
bool ibiBufferFilled = false;

int stressState = 0;  // 1–4 scale


// SETUP
void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println("\nConnecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.println("TCP server running on port 5000");
}


void loop() {

  // Accept client connection
  if (!client || !client.connected()) {
    client = server.available();
    if (client) Serial.println("Client connected!");
  }

  readPulseSensor();    // Manual beat detection
  sendStressState();    // Send processed result

  delay(5); // ~200 Hz sampling
}


// PULSE PROCESSING
void readPulseSensor() {
  int rawValue = analogRead(SENSOR_PIN);

  unsigned long currentTime = millis();

  // Beat detection using threshold crossing
  if (rawValue > THRESHOLD && !beatDetected) {
    beatDetected = true;

    if (lastBeatTime > 0) {
      IBI = currentTime - lastBeatTime;   // Calculate Inter-Beat Interval
      storeIBI(IBI);
      computeStress();
    }

    lastBeatTime = currentTime;
  }

  // Reset after falling below threshold
  if (rawValue < THRESHOLD - 50) {
    beatDetected = false;
  }
}



// IBI STORAGE
void storeIBI(unsigned long ibi) {
  ibiList[ibiIndex] = ibi;
  ibiIndex++;

  if (ibiIndex >= MAX_IBI_SAMPLES) {
    ibiIndex = 0;
    ibiBufferFilled = true;
  }
}



// RMSSD & STRESS CALCULATION
void computeStress() {
  if (!ibiBufferFilled) return;

  double sumSqDiff = 0;

  for (int i = 0; i < MAX_IBI_SAMPLES - 1; i++) {
    float diff = (float)ibiList[i + 1] - (float)ibiList[i];
    sumSqDiff += diff * diff;
  }

  float rmssd = sqrt(sumSqDiff / (MAX_IBI_SAMPLES - 1));

  // Classify stress level
  if (rmssd > 60) stressState = 1;          // Relaxed
  else if (rmssd > 40) stressState = 2;     // Mild Stress
  else if (rmssd > 20) stressState = 3;     // Stressed
  else stressState = 4;                     // High Stress
}


// TCP SENDING
void sendStressState() {
  if (client && client.connected()) {
    client.println(stressState);
  }
}
