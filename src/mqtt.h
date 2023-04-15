#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
// #include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <Wire.h>
#include <WiFi.h>
#include <AsyncMqttClient.h>
#include "arduino_secrets.h"
#include "storage.h"
#include <ArduinoJson.h>

class Mqtt
{
public:
    Mqtt();
    void setup();
    void connect();
    void publish(String topic, String msg);
    void subscribe(String topic);
    void setIsAlarmLevel(bool val);
    void setIsSaveData(bool val);
    bool getIsLogging();
    bool connected();
    bool getIsNoiseCalibrating();
    bool getIsLoggingCalibrating();
    bool getIsSaveData();
    bool getIsAlarmLevel();
    bool isUpdateProfile();
    bool isUpdateFW();
    int getVersionToDownload();
    float getAlarmLevel();
    float getWarningLevel();
    profile_t getProfile();

private:
    static void handelSetupProfileTopic(char *payload, StaticJsonDocument<600> doc);
    static void handelLoggingModeTopic(char *payload, StaticJsonDocument<600> doc);
    static void checkIfRunCalLogging(StaticJsonDocument<600> doc);
    static void checkIfRunLogging(StaticJsonDocument<600> doc);
    static void checkIfRunNoiseLogging(StaticJsonDocument<600> doc);
    static void connectToMqtt();
    static void onMqttConnect(bool sessionPresent);
    static void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
    static void onMqttSubscribe(uint16_t packetId, uint8_t qos);
    static void onMqttUnsubscribe(uint16_t packetId);
    static void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
    static void onMqttPublish(uint16_t packetId);
    static void WiFiEvent(WiFiEvent_t event);

    static const char *SETUP_PROFILE_PATH; // get Profile from server and save it?
    static const char *SETUP_ALARM_PATH;   // alarm/warining level float
    static const char *SETUP_RUN_CAL_PATH; // run logging nosie/elevator/normal
    static const char *UPDATE_PATH;        // true/falsef
    static float _alarmLevel;
    static float _warningLevel;
    static float _isAlarmLevel;
    static bool _isRunLogging;
    static bool _isCalibratingLogging;
    static bool _isNoiseLogging;
    static bool _isUpdateProfile;
    static bool _isDownload;
    static int installedFW;
    static bool _isSaveData;
    static profile_t _profile;
    static Storage _storeProfile;
    static AsyncMqttClient _mqttClient;
};
#endif