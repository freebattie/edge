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
profile_t profile;
sensor_flag_t lastFlags;
sensor_flag_t currentFlags;
live_values_t liveValues;
light_data_t totalHoursOfLight;
mqtt_testing_t mqttTesting;
void setupTimers();
void handelConnections();
void readLiveSensorValues();
void transmittOnValueChanged();
bool isAlarmLimitReached(float val, int limit, bool isUpperLimit);
void transmittLiveValues();
void setup()
{
  setupTimers();
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
  profile = store.getProfile();
  Serial.println("SETUP DONE");
  Serial.println(profile.city);
}

void loop()
{
  handelConnections();

  rgb.handelLight();
  tempSensor.handelSensor();
  room.handelSensors();
 
  // tempSensor.checkForHeaterFailure(temp+5);
  // rgb.setState(ColorState::FIND);
  rgb.handelLight();
  lux.handelSensor();
  if (lastFlags.isLogging)
  {
    readLiveSensorValues();
    // send data on change
    transmittOnValueChanged();
    // send value data
    transmittLiveValues();
    // send collected sun data when min sun reached;
    if (lux.getSendData() || mqttTesting.sendLightData)
    {

      StaticJsonDocument<256> doc;
      String msg;
      light_data_t data = lux.getLightData();
      doc["sunlight"] = data.sunLightHours;
      doc["lamplight"] = data.lampLightHours;
      doc["total"] = data.totalHours();
      serializeJson(doc, msg);
      mqttClient.publish(profile.location + "/light", msg);
      lux.setSendData(false);
    }
  }

  if (mqttClient.isUpdateProfile())
  {
    profile = store.getProfile();
    mqttClient.setIsUpdateProfile(false);
  }
  if (mqttClient.isUpdateFW())
  {
    wifiOta.update(profile);
    mqttClient.setIsUpdateFW(false);
  }
  if (mqttClient.getIsDissconectAlarm())
  {
   rgb.setState(ALARM);
  }
  else
  {
     rgb.setState(ALARMOFF);
  }
  
  delay(400);
}

void readLiveSensorValues()
{
  ColorState state = rgb.getState();
  currentFlags.isDeviceAlarm = rgb.getState() == ALARM ? true : false;
  currentFlags.isDoorOpen = room.getIsDoorOpen();
  currentFlags.isFindMe = rgb.getState() == FIND ? true : false;
  currentFlags.isHeater = tempSensor.getIsHeaterOn();
  currentFlags.isHeaterFailed = tempSensor.checkForHeaterFailure(HEATER_FAILURE_TEMP);
  float humid = tempSensor.getHumidity();
  float temp = tempSensor.getTemp();
  liveValues.humidity = humid;
  liveValues.temp = temp;
  liveValues.lux = lux.getLux();

  currentFlags.isHighHumidityAlarm = isAlarmLimitReached(humid, 90, true);
  currentFlags.isLowHumidityAlarm = isAlarmLimitReached(humid, 50, false);
  currentFlags.isHighHumidityWarning = isAlarmLimitReached(humid, 65, true);
  currentFlags.isLowHumidityWarning = isAlarmLimitReached(humid, 55, false);

  currentFlags.isHighTempAlarm = isAlarmLimitReached(temp, 33, true);
  currentFlags.isLowTempAlarm = isAlarmLimitReached(temp, 14, false);
  currentFlags.isHighTempWarning = isAlarmLimitReached(temp, 30, true);
  currentFlags.isLowTempWarning = isAlarmLimitReached(temp, 24, false);

  currentFlags.isWindowOpen = room.getIsWindowOpen();
  lastFlags.isWindowOpenAlarm = false; // TODO GET WEATHER AND REOPRT
  currentFlags.isLogging = store.getProfile().location.length() > 0;

  // TODO: CALL WEATHER API AND GET WEATHER
  // TODO: GET IF LOGGING OR NOT??
}

bool transmittGivenValueOnChange(bool oldVal, bool newVal, String type, String name)
{
  StaticJsonDocument<256> doc;
  String msg;
  if (oldVal != newVal)
  {
    doc["type"] = type;
    doc["name"] = name;
    doc["status"] = newVal;

    serializeJson(doc, msg);
    mqttClient.publish(profile.location + "/alarm", msg);
    delay(1000);
    doc.clear();
    return newVal;
  }
  doc.clear();
  return oldVal;
}

void transmittOnValueChanged()
{
 

  lastFlags.isDeviceAlarm = transmittGivenValueOnChange(lastFlags.isDeviceAlarm, currentFlags.isDeviceAlarm, "ALARM", "device");
  lastFlags.isDoorOpen = transmittGivenValueOnChange(lastFlags.isDoorOpen, currentFlags.isDoorOpen, "ALARM", "door");
  lastFlags.isFindMe = transmittGivenValueOnChange(lastFlags.isFindMe, currentFlags.isFindMe, "STATUS", "findme");
  lastFlags.isHeater = transmittGivenValueOnChange(lastFlags.isHeater, currentFlags.isHeater, "STATUS", "heater");
  lastFlags.isHeaterFailed = transmittGivenValueOnChange(lastFlags.isHeaterFailed, currentFlags.isHeaterFailed, "ALARM", "heater");

  lastFlags.isHighHumidityAlarm = transmittGivenValueOnChange(lastFlags.isHighHumidityAlarm, currentFlags.isHighHumidityAlarm, "ALARM", "high_humidity");
  lastFlags.isLowHumidityAlarm = transmittGivenValueOnChange(lastFlags.isLowHumidityAlarm, currentFlags.isLowHumidityAlarm, "ALARM", "low_humidity");
  lastFlags.isHighHumidityWarning = transmittGivenValueOnChange(lastFlags.isHighHumidityWarning, currentFlags.isHighHumidityWarning, "WARNING", "high_humidity");
  lastFlags.isLowHumidityWarning = transmittGivenValueOnChange(lastFlags.isLowHumidityWarning, currentFlags.isLowHumidityWarning, "WARNING", "low_humidity");

  lastFlags.isHighTempAlarm = transmittGivenValueOnChange(lastFlags.isHighTempAlarm, currentFlags.isHighTempAlarm, "ALARM", "high_temp");
  lastFlags.isLowTempAlarm = transmittGivenValueOnChange(lastFlags.isLowTempAlarm, currentFlags.isLowTempAlarm, "ALARM", "low_temp");
  lastFlags.isHighTempWarning = transmittGivenValueOnChange(lastFlags.isHighTempAlarm, currentFlags.isHighTempAlarm, "WARNING", "high_temp");
  lastFlags.isLowTempWarning = transmittGivenValueOnChange(lastFlags.isLowTempWarning, currentFlags.isLowTempWarning, "WARNING", "low_temp");

  lastFlags.isLogging = transmittGivenValueOnChange(lastFlags.isLogging, currentFlags.isLogging, "STATUS", "logging");
  lastFlags.isWindowOpen = transmittGivenValueOnChange(lastFlags.isWindowOpen, currentFlags.isWindowOpen, "STATUS", "window");
  lastFlags.isWindowOpenAlarm = transmittGivenValueOnChange(lastFlags.isWindowOpenAlarm, currentFlags.isWindowOpenAlarm, "ALARM", "window");
}
bool isAlarmLimitReached(float val, int limit, bool isUpperLimit)
{
  if (isUpperLimit)
  {
    if (val > limit)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    if (val < limit)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
}
void transmittLiveValues()
{
  StaticJsonDocument<256> doc;
  String msg;
  doc["device"] = profile.deviceName;
  doc["lux"] = liveValues.lux;
  doc["temp"] = liveValues.temp;
  doc["humidity"] = liveValues.humidity;
  serializeJson(doc, msg);
  mqttClient.publish(profile.location + "/live", msg);
  
  doc.clear();
}
void handelTransmittingBoolState()
{
  if (rgb.getState() == ALARM)
  {
    /* code */
  }
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
  else if (!mqttClient.connected() && wifiOta.status() == WL_CONNECTED && mqttTimer.checkInterval() == RUNCODE)
  {
    mqttTimer.reset();
    Serial.println("Restarting device standby .. ");
    delay(2000);
    ESP.restart();
  }
  else if (!mqttClient.connected() && wifiOta.status() == WL_CONNECTED)
  {
    Serial.println("dissconnected from mqtt.. ");
    delay(200);
  }
}