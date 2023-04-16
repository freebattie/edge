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
#include "rgbColor.h"
#include "tempSensor.h"
#include "roomStatus.h"
#include "lightSensor.h"

Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BGR);
#define HALL_SENSOR_PIN 1

Timer wifiTimer = Timer();
Timer mqttTimer = Timer();
Ota wifiOta = Ota();
Mqtt mqttClient = Mqtt();
Storage store = Storage();
RgbColor rgb = RgbColor(strip);
TempSensor tempSensor = TempSensor(rgb);
RoomStatus room = RoomStatus(rgb);
LightSensor lux = LightSensor(rgb);
void setupTimers();
void handelConnections();

void setup()
{
  setupTimers();
  // put your setup code here, to run once:

  Serial.begin(9800);
  delay(4000);
  delay(1000);
  mqttClient.setup();
  wifiOta.connectWiFi();
  delay(1000);
  rgb.setup();
  tempSensor.setup();
  room.setup();
  rgb.setState(ColorState::NORMAL);

  store.start();
  lux.setLuxLevel(350);
  lux.setMinHours(8);
  
  lux.setup();
  profile_t profile = store.getProfile();

  // mqttConnect.setup();

  Serial.println("SETUP DONE");
  Serial.println(profile.city);
}

void loop()
{
  handelConnections();

  rgb.handelLight();
  tempSensor.handelSensor();
  room.handelSensors();
  tempSensor.setIsHeaterOn(false);
  // tempSensor.checkForHeaterFailure(temp+5);
  // rgb.setState(ColorState::FIND);
  rgb.handelLight();
  lux.handelSensor();

  delay(400);
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
  else if (!mqttClient.connected() &&  wifiOta.status() == WL_CONNECTED && mqttTimer.checkInterval() == RUNCODE)
  {
    mqttTimer.reset();
    Serial.println("Restarting device standby .. ");
    delay(2000);
    ESP.restart();
  } 
  else if (!mqttClient.connected() &&  wifiOta.status() == WL_CONNECTED)
  {
    Serial.println("dissconnected from mqtt.. ");
    delay(200);
  }
}