

#include "timer.h"

#include "ota.h"
#include "mqtt.h"
#include "storage.h"
#include <Arduino.h>
#include "rgbColor.h"
#include "tempSensor.h"
#include "roomStatus.h"
#include "lightSensor.h"
#include "SPIFFS.h"

Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BGR);
#define HALL_SENSOR_PIN 1

Timer wifiTimer = Timer();
Timer mqttTimer = Timer();
Timer apiWeatherTimer = Timer();

Timer restartWifiManagerTimer = Timer();
Timer transmittingLiveIntervall = Timer();
Timer transmittingChangedIntervall = Timer();
Timer LoggerMsgTimer = Timer();
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

ColorState oldState = NORMAL;
bool isblockProfile = false;
const char *URL = "https://api.openweathermap.org/data/2.5/weather?q=%s&appid=%s&units=metric";
int outsideTemp;
String sky;

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
  Serial.begin(115200);
  delay(4000);
  SPIFFS.begin(false);
  if (wifiOta.runMqttSetup())
  {
    String mqttserver = wifiOta.getMqttServerIP();
    String mqttserverPort = wifiOta.getMqttServerPort();
    Serial.println("IP AND PORT");
    Serial.println(mqttserver);
    Serial.println(mqttserverPort);
    mqttClient.setup(mqttserver, mqttserverPort);
  }

  wifiOta.setup();

  delay(1000);
  if (wifiOta.setupDone())
  {

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

    String buffer = GetWeather();
    StaticJsonDocument<1024> doc;

    DeserializationError error = deserializeJson(doc, buffer);

    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }
    outsideTemp = doc["main"]["temp"];
    Serial.println("outsideTemp");
    Serial.println(outsideTemp);
    sky = doc["weather"][0]["main"].as<String>();
    doc.clear();
  }
  else
    Serial.println("setup server running");
}

void loop()
{
  if (wifiOta.setupDone())
  {
    LoggerMsgTimer.setInterval(3000);
    LoggerMsgTimer.start();
    handelConnections();
    rgb.handelLight();
    tempSensor.handelSensor(); // TODO HANDEL ALARM??
    room.handelSensors();
    rgb.handelLight();
    lux.handelSensor();
    readLiveSensorValues();
    // send data on change
    transmittOnValueChanged();
    if (LoggerMsgTimer.checkInterval() == RUNCODE)
    {
      LoggerMsgTimer.reset();
      if (lastFlags.isLogging || currentFlags.isLogging)
      {
        Serial.println("is logging");
      }
      else
      {
        Serial.println("is not logging");
      }
    }
    if (transmittingLiveIntervall.checkInterval() == RUNCODE)
    {
      // Serial.println("intervall");
      transmittingLiveIntervall.reset();

      if (lastFlags.isLogging || currentFlags.isLogging)
      {

        handelData();
      }
    }

    handelDownloadFW();
    handelProfileUpdate();
    if (mqttClient.isUpdateProfile() && !isblockProfile)
    {
      mqttClient.setIsUpdateProfile(false);
      // ESP.restart();
    }
    handelDeviceAlarm();
    handelFindMe();
    delay(100);
  }
  else
  {
    Serial.println("please go to webpage and and turn off wifimanager if setup is done");
    delay(300);
  }
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
    Serial.println("is updating profile");
    profile = store.getProfile();

    currentFlags.isLogging = profile.location.length() > 0;
  }
}

void handelDownloadFW()
{
  if (mqttClient.isUpdateFW())
  {
    isblockProfile = true;
    Serial.println("is downlaoding new fw");
    wifiOta.update(store.getProfile());
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

  // send value data
  transmittLiveValues();
  // send collected sun data when min sun reached;
  if (lux.getSendData())
  {

    StaticJsonDocument<256> doc;
    String msg;
    light_data_t data = lux.getLightData();
    doc["sunLight"] = data.sunLightHours;
    doc["lampLight"] = data.lampLightHours;
    doc["total"] = data.totalHours();
    serializeJson(doc, msg);
    mqttClient.publish("locations/" + profile.location + "/light", msg);
    lux.setSendData(false);
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
  // reads the values and if prop 3 is set to true it checks vs upper limt and false it checks vs lower limit
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
    StaticJsonDocument<1024> doc;

    DeserializationError error = deserializeJson(doc, buffer);

    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }

    outsideTemp = doc["main"]["temp"];
    sky = doc["weather"]["main"].as<String>();
    doc.clear();
  }

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

  // TODO: CALL WEATHER API AND GET WEATHER
}
String GetWeather()
{
  char url_path[120];
  profile = store.getProfile();
  Serial.println(profile.city);
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
    if (transmittingChangedIntervall.checkInterval() == RUNCODE)
    {
      transmittingChangedIntervall.reset();
      doc["type"] = type;
      doc["name"] = name;
      doc["status"] = newVal;

      serializeJson(doc, msg);
      mqttClient.publish("locations/" + profile.location + "/alarm", msg);
      delay(ON_CHANGE_SPACING);
      doc.clear();
      return newVal;
    }
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
  lastFlags.isWindowOpenAlarm = transmittGivenValueOnChange(lastFlags.isWindowOpenAlarm, currentFlags.isWindowOpenAlarm, "ALARM", "window_open");
  lastFlags.isWindowClosedAndHotOutside = transmittGivenValueOnChange(lastFlags.isWindowClosedAndHotOutside, currentFlags.isWindowClosedAndHotOutside, "ALARM", "window_closed");
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
  doc["deviceName"] = profile.deviceName;
  doc["lux"] = liveValues.lux;
  doc["temp"] = liveValues.temp;
  doc["humidity"] = liveValues.humidity;
  profile = store.getProfile();
  serializeJson(doc, msg);
  mqttClient.publish("locations/" + profile.location + "/live", msg);

  doc.clear();
}

void setupTimers()
{
  transmittingLiveIntervall.setInterval(500);
  transmittingLiveIntervall.start();
  transmittingChangedIntervall.setInterval(300);
  transmittingChangedIntervall.start();

  apiWeatherTimer.setInterval(10 * 60 * 1000);
  apiWeatherTimer.start();

  wifiTimer.setInterval(5000);
  wifiTimer.start();

  mqttTimer.setInterval(5000);
  mqttTimer.start();
  restartWifiManagerTimer.setInterval(1000 * 60 * 5);
  restartWifiManagerTimer.stop();
}

void handelConnections()
{
  if (wifiOta.status() != WL_CONNECTED && wifiTimer.checkInterval() == RUNCODE)
  {
    // go to http://deviceip and reset wifimanger to change network
    Serial.println("Lost connection to wifi.. ");

    delay(2000);
    if (restartWifiManagerTimer.checkInterval() == STOPPED)
    {
      restartWifiManagerTimer.reset();
      restartWifiManagerTimer.start();
    }
    if (restartWifiManagerTimer.checkInterval() == RUNCODE)
    {
      Serial.println("Restarting WifiManger please config device again. ");
      delay(3000);
      wifiOta.reset();
    }
    Serial.println("go to http://deviceip and reset wifimanger to change network ");
    Serial.println("trying to reconnect..  ");
    wifiOta.reConnect();
  }
  else if (!mqttClient.connected() &&
           mqttTimer.checkInterval() == RUNCODE &&
           wifiOta.setupDone())
  {
    if (restartWifiManagerTimer.checkInterval() == STOPPED)
    {
      restartWifiManagerTimer.reset();
      restartWifiManagerTimer.start();
    }
    if (restartWifiManagerTimer.checkInterval() == RUNCODE)
    {
      wifiOta.reset();
    }

    mqttTimer.reset();
    Serial.println("Connected to wifi but no Mqtt server found.. ");

    delay(2000);
    // ESP.restart();
  }
  else if (!mqttClient.connected() && wifiOta.status() == WL_CONNECTED)
  {

    Serial.println("dissconnected from mqtt.. ");

    delay(200);
  }
  else if (restartWifiManagerTimer.checkInterval() == RUNNING && mqttClient.connected())
  {
    restartWifiManagerTimer.reset();
    restartWifiManagerTimer.stop();
  }
}
#pragma endregion