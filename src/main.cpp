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

#include <WiFi.h>
#include <HTTPClient.h>
Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BGR);
#define HALL_SENSOR_PIN 1

Timer wifiTimer = Timer();
Timer mqttTimer = Timer();
Timer apiWeatherTimer = Timer();
Timer mqttIntervall = Timer();
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
ColorState oldState = NORMAL;
const char *URL = "https://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric";
void setupTimers();
void handelConnections();
void readLiveSensorValues();
void transmittOnValueChanged();
bool isAlarmLimitReached(float val, int limit, bool isUpperLimit);
void transmittLiveValues();
void handelData();
void handelDeviceAlarm();
void handelProfileUpdate();
String GetWeather();
void handelDownloadFW();
void handelFindMe();
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
  lastFlags.isLogging = store.getProfile().location.length() > 0;
  int weathertimer;
  int senddataTimer;
}

void loop()
{

  handelConnections();
  rgb.handelLight();
  tempSensor.handelSensor();
  room.handelSensors();
  rgb.handelLight();
  lux.handelSensor();

  if (lastFlags.isLogging || currentFlags.isLogging)
  {
    handelData();
  }

  handelProfileUpdate();
  handelDownloadFW();
  handelDeviceAlarm();
  handelFindMe();
  delay(400);
}

#pragma region Methods
void handelFindMe()
{
  if (mqttClient.getIsFindMe())
  {
    oldState = rgb.getState();
    rgb.setState(FIND);
  }
  else
  {
    rgb.setState(oldState != FIND ? oldState : FINDOFF);
  }
}
void handelProfileUpdate()
{
  if (mqttClient.isUpdateProfile())
  {
    profile = store.getProfile();
    mqttClient.setIsUpdateProfile(false);
    currentFlags.isLogging = profile.location.length() > 0;
  }
}
void handelDownloadFW()
{
  if (mqttClient.isUpdateFW())
  {
    wifiOta.update(profile);
    mqttClient.setIsUpdateFW(false);
  }
}
void handelDeviceAlarm()
{
  if (mqttClient.getIsDissconectAlarm())
  {
    rgb.setState(ALARM);
  }
  else
  {
    rgb.setState(ALARMOFF);
  }
}
void handelData()
{
  if (mqttIntervall.checkInterval() == RUNCODE)
  {
    mqttIntervall.reset();
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
      mqttClient.publish("locations/" + profile.location + "/light", msg);
      lux.setSendData(false);
    }
  }
}

void readLiveSensorValues()
{

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

  currentFlags.isHighHumidityAlarm = isAlarmLimitReached(humid, HUMID_H_ALARM, true);
  currentFlags.isLowHumidityAlarm = isAlarmLimitReached(humid, HUMID_L_ALARM, false);
  currentFlags.isHighHumidityWarning = isAlarmLimitReached(humid, HUMID_H_WARN, true);
  currentFlags.isLowHumidityWarning = isAlarmLimitReached(humid, HUMID_L_WARN, false);

  currentFlags.isHighTempAlarm = isAlarmLimitReached(temp, TEMP_H_ALARM, true);
  currentFlags.isLowTempAlarm = isAlarmLimitReached(temp, TEMP_L_ALARM, false);
  currentFlags.isHighTempWarning = isAlarmLimitReached(temp, TEMP_H_WARN, true);
  currentFlags.isLowTempWarning = isAlarmLimitReached(temp, TEMP_L_WARN, false);

  currentFlags.isWindowOpen = room.getIsWindowOpen();

  currentFlags.isLogging = store.getProfile().location.length() > 0;

  if (apiWeatherTimer.checkInterval() == RUNCODE)
  {
    apiWeatherTimer.reset();
    String buffer = GetWeather();
    StaticJsonDocument<256> doc;
    deserializeJson(doc, buffer);

    int outsideTemp = doc["main"]["temp"];
    String sky = doc["weather"]["main"];

    if (outsideTemp < HEATER_FAILURE_TEMP && currentFlags.isWindowOpen)
    {
      currentFlags.isWindowOpenAlarm = true;
    }
    else
    {
      currentFlags.isWindowOpenAlarm = false;
    }

    if (outsideTemp > HEATER_FAILURE_TEMP && sky == "Clear")
    {
      currentFlags.isWindowClosedAndHotOutside = true;
    }
    else
    {
      currentFlags.isWindowClosedAndHotOutside = false;
    }
  }

  // TODO: CALL WEATHER API AND GET WEATHER
}
String GetWeather()
{
  char url_path[120];
  snprintf(url_path, sizeof(url_path), URL, profile.city, WEATHER_API_KEY);
  HTTPClient http;

  http.begin(url_path);

  int resCode = http.GET();

  String data = "";

  if (resCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(resCode);
    data = http.getString();
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(resCode);
  }

  // Free resources
  http.end();
  return data;
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
    mqttClient.publish("locations/" + profile.location + "/alarm", msg);
    delay(ON_CHANGE_SPACING);
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
  mqttClient.publish("locations/" + profile.location + "/live", msg);

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
  mqttIntervall.setInterval(1000);
  apiWeatherTimer.setInterval(10000);
  apiWeatherTimer.start();
  mqttIntervall.start();
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
#pragma endregion