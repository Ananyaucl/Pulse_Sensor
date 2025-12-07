#include <SPI.h>
#include <WiFiNINA.h>
#include "arduino_secrets.h" 
#include <utility/wifi_drv.h>
#include "LEDAnimationFunctions.h"

const char* ssid          = SECRET_SSID;
const char* password      = SECRET_PASS;

int status = WL_IDLE_STATUS;   
IPAddress server(192,168,0,40); 

WiFiClient client;

void setup() {
  Serial.begin(9600);
  Serial.println("Attempting to connect to network");

  status = WiFi.begin(ssid, password);
  if (status != WL_CONNECTED){
    Serial.println("Unable to connect to network");
    while(true);
  }
  else{
    Serial.println("Connected to WiFi");
    Serial.println("\nStarting connection...");

    if (client.connect(server, 5000)) {
      Serial.println("connected");
      Serial.println(WiFi.localIP());
    }
  }
  initStressDisplay();
}

void loop() {

  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();

    if (client.connect(server, 5000)) {
      Serial.println("Reconnected!");
    }
    return;
  }
  while (client.available()) {
    char c = client.read();
     if (c >= '0' && c <= '9') {   // ONLY accept valid digits
        int stateValue = c - '0';
        setStressLevel(stateValue);          // ← instantly switches animation
        updateStressDisplay();
    }
  }
}
