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
    void setIsUpdateFW(bool download);
    void setIsUpdateProfile(bool update);
    bool connected();
    bool isUpdateProfile();
    bool isUpdateFW();
    bool getIsFindMe();
    profile_t getProfile();
    bool getIsDissconectAlarm();
    

private:
    static bool _isMqttDisconectAlarm;
    static void handelSetupProfileTopic(char *payload, StaticJsonDocument<600> doc);
    static void connectToMqtt();
    static void onMqttConnect(bool sessionPresent);
    static void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
    static void onMqttSubscribe(uint16_t packetId, uint8_t qos);
    static void onMqttUnsubscribe(uint16_t packetId);
    static void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
    static void onMqttPublish(uint16_t packetId);
    static void WiFiEvent(WiFiEvent_t event);

    static const char *DEVICES;
    static const char *DEVICE;
    static const char *DEVICE_SETUP_PROFILE_TOPIC; // get Profile from server and save it? // {version: "-1/1/2",build:"dev/prod"}
    static const char *UPDATE_TOPIC;
    static const char *LOCATION_ALARM_TOPIC; //{name: "heater/heater failure/door",type: "warning/alarm/status", status: true/false}
    static const char *LOCATION_LOGGING_TOPIC;
    static const char *LOCATIONS_UPDATE;

    static bool _isUpdateProfile;
    static bool _isDownload;
    static bool _isFindMe;
    static bool _isblockProfile;
    static profile_t _profile;
    static Storage _storeProfile;
    static AsyncMqttClient _mqttClient;
};
#endif