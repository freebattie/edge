#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include "timer.h"
#include "arduino_secrets.h"
#include <WiFi.h>
#include <AsyncMqttClient.h>
#include "ota.h"
#include "mqtt.h"
#include "storage.h"
#include <Arduino.h>

#define HALL_SENSOR_PIN 1

bool isDoorOpen = false;

Adafruit_LIS3DH lis = Adafruit_LIS3DH();
Timer wifiTimer = Timer();
Timer mqttTimer = Timer();
Ota wifiOta = Ota();
Storage store = Storage();


void setup() {
  // put your setup code here, to run once:
   pinMode(HALL_SENSOR_PIN, INPUT);
  Serial.begin(9800);
  delay(4000);

  store.start();

  profile_t profile = store.getProfile();

 
  setupTimers();

  //mqttConnect.setup();
  delay(1000);
  wifiOta.connectWiFi();
  delay(1000);
  Serial.println("SETUP DONE");
}

void loop() {
  // put your main code here, to run repeatedly:
}
void setupTimers()
{
 
  wifiTimer.setInterval(5000);
  wifiTimer.start();

  mqttTimer.setInterval(5000);
  mqttTimer.start();
}
void handelConnections()
{
  if (wifiOta.status() != WL_CONNECTED && wifiTimer.checkInterval() == RUNCODE)
  {
    wifiTimer.reset();
    wifiOta.connectWiFi();
  }
  
  else if (/*!mqttConnect.connected() && */ wifiOta.status() == WL_CONNECTED && mqttTimer.checkInterval() == RUNCODE)
  {
    mqttTimer.reset();
    Serial.println("Restarting device standby .. ");
    delay(2000);
    ESP.restart();
  }
  else if (/*!mqttConnect.connected() && */wifiOta.status() == WL_CONNECTED)
  {
    Serial.println("dissconnected from mqtt.. ");
    delay(200);
  }
}