#include "mqtt.h"

#pragma region member definition
const char *Mqtt::DEVICES = "devices";
const char *Mqtt::DEVICE = "devices/%s";
const char *Mqtt::DEVICE_SETUP_PROFILE_TOPIC = "devices/%s/profile"; // get Profile from server and save it?
const char *Mqtt::UPDATE_TOPIC = "update";
const char *Mqtt::LOCATIONS_UPDATE = "locations/update";
const char *Mqtt::LOCATION_ALARM_TOPIC = "locations/%s/alarm";
const char *Mqtt::LOCATION_LOGGING_TOPIC = "locations/%s/logging";
// const char *Mqtt::SETUP_ALARM_PATH = "setup/Levels";    // alarm/warining level float
// const char *Mqtt::SETUP_RUN_CAL_PATH = "setup/cal/run"; // run logging nosie/elevator/normal

bool Mqtt::_isUpdateProfile = false;
bool Mqtt::_isDownload = false;
bool Mqtt::_isMqttDisconectAlarm = false;
bool Mqtt::_isFindMe = false;
bool Mqtt::_isblockProfile = false;
profile_t Mqtt::_profile;
Storage Mqtt::_storeProfile;
AsyncMqttClient Mqtt::_mqttClient;
#pragma endregion

#pragma region public methods
Mqtt::Mqtt()
{
    _storeProfile = Storage();
}

void Mqtt::subscribe(String topic)
{
    _mqttClient.subscribe(topic.c_str(), 0);
}
void Mqtt::setIsUpdateFW(bool download)
{
    _isDownload = download;
}
void Mqtt::publish(String topic, String msg)
{

    _mqttClient.publish(topic.c_str(), 0, false, msg.c_str());
}
void Mqtt::WiFiEvent(WiFiEvent_t event)
{
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch (event)
    {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        connectToMqtt();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi lost connection");
        break;
    }
}

void Mqtt::setup()
{
    Serial.println("SETUP storage in MQTT");
    _storeProfile.start();
    _profile = _storeProfile.getProfile();

    WiFi.onEvent(WiFiEvent);
    _mqttClient.onConnect(onMqttConnect);
    _mqttClient.onDisconnect(onMqttDisconnect);
    _mqttClient.onSubscribe(onMqttSubscribe);
    _mqttClient.onUnsubscribe(onMqttUnsubscribe);
    _mqttClient.onMessage(onMqttMessage);
    _mqttClient.onPublish(onMqttPublish);
    _mqttClient.setClientId(_profile.deviceName.c_str());
    _mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    _mqttClient.setKeepAlive(120);

    Serial.println("SETUP DONE MQTT");
}

void Mqtt::connect()
{
    Serial.println("re connecting to Mqtt server");
    Serial.print("using password ");
    Serial.println(_profile.mqtt_pass.c_str());
    //_mqttClient.setClientId(_profile.mqtt_username.c_str());
    _mqttClient.disconnect();
    if (_mqttClient.connected())
    {
        _mqttClient.disconnect();
    }

    _mqttClient.setCredentials(_profile.mqtt_username.c_str(), _profile.mqtt_pass.c_str());
    _mqttClient.connect();
}

bool Mqtt::isUpdateProfile()
{
    if (_isblockProfile)
        return false;
    else
        return _isUpdateProfile;
}

bool Mqtt::isUpdateFW()
{
    return _isDownload;
}

bool Mqtt::getIsFindMe()
{
    return _isFindMe;
}

void Mqtt::setIsUpdateProfile(bool update)
{
    _isUpdateProfile = update;
}

bool Mqtt::connected()
{
    return _mqttClient.connected();
}

profile_t Mqtt::getProfile()
{
    return _profile;
}
bool Mqtt::getIsDissconectAlarm()
{
    return _isMqttDisconectAlarm;
}
#pragma endregion

#pragma region private methods
void Mqtt::connectToMqtt()
{
    Serial.println("Connecting to MQTT...");
    _mqttClient.setCredentials(_profile.mqtt_username.c_str(), _profile.mqtt_pass.c_str());
    _mqttClient.connect();

    Serial.println("MQTT CONNECTED");
}

void Mqtt::handelSetupProfileTopic(char *payload, StaticJsonDocument<600> doc)
{

    _profile.location = doc["location"].as<String>();
    _profile.mqtt_username = doc["deviceName"].as<String>();
    _profile.mqtt_pass = doc["mqttPass"].as<String>();
    //_profile.city = doc["city"].as<String>();
    int fw = doc["fw"].as<int>();
    _profile.isAutoUpdateOn = doc["auto"].as<bool>();
    String build = doc["build"].as<String>();
    Serial.print("before u were  is dev?");
    Serial.println(_profile.build);
    if (_profile.isAutoUpdateOn)
    {

        _profile.build = build;
        _storeProfile.saveProfile(_profile);
    }
    else if (fw != _profile.fw || build != _profile.build)
    {
        Serial.println("update manual ");
        Serial.println(_profile.isAutoUpdateOn);
        Serial.println("FW WE GOT WAS");
        Serial.println(fw);
        Serial.println("and is now");
        Serial.println(_profile.fw);
        Serial.println("changing fw..");
        _profile.fw = fw;
        _isDownload = true;
        _profile.build = build;
        _isblockProfile = true;
    }

    //{"deviceName": "name", "location": "location2", "build" : "dev", "city": "oslo", "fw":"1","auto":"true"} ;
    Serial.print("this is dev?");
    Serial.println(_profile.build);
    _storeProfile.saveProfile(_profile);
    _profile = _storeProfile.getProfile();
    Serial.println(_profile.mqtt_pass);
    _isUpdateProfile = true;

    // update city for new location
    StaticJsonDocument<256> docSend;

    docSend["deviceName"] = _profile.deviceName;

    String msg;
    serializeJson(docSend, msg);

    _mqttClient.publish("devices", 1, false, msg.c_str());
}

void Mqtt::onMqttConnect(bool sessionPresent)
{
    _profile = _storeProfile.getProfile();
    Serial.println("connected to MQTT SERVER ");
    StaticJsonDocument<256> doc;
    _isMqttDisconectAlarm = false;
    doc["deviceName"] = _profile.deviceName;

    String msg;
    serializeJson(doc, msg);
    _mqttClient.publish("devices", 1, false, msg.c_str());
    doc.clear();
    const char *deviceName = _profile.deviceName.c_str();

    char deviceSetupTopic[32]; // devices/:name/profile

    snprintf(deviceSetupTopic, sizeof(deviceSetupTopic), DEVICE_SETUP_PROFILE_TOPIC, deviceName);

    char deviceTopic[32]; // devices/:name
    snprintf(deviceTopic, sizeof(deviceTopic), DEVICE, deviceName);

    if (_profile.location.length() > 1)
    {
        const char *deviceLocation = _profile.location.c_str();
        char deviceLocationAlarm[32];
        snprintf(deviceLocationAlarm, sizeof(deviceLocationAlarm), LOCATION_ALARM_TOPIC, deviceLocation);
        _mqttClient.subscribe(deviceLocationAlarm, 1);
        char deviceLocationLogging[32];
        snprintf(deviceLocationLogging, sizeof(deviceLocationLogging), LOCATION_LOGGING_TOPIC, deviceLocation);
        _mqttClient.subscribe(deviceLocationLogging, 1);
    }
    _mqttClient.subscribe("findMe", 1);
    _mqttClient.subscribe("devices", 1);
    _mqttClient.subscribe(deviceTopic, 1);
    _mqttClient.subscribe(deviceSetupTopic, 1);
    _mqttClient.subscribe(UPDATE_TOPIC, 1);
    _mqttClient.subscribe(LOCATIONS_UPDATE, 1);
}
// static methods
void Mqtt::onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
    Serial.println("Disconnected from MQTT.");
    _isMqttDisconectAlarm = true;
    Serial.println(int(reason));

    if (WiFi.isConnected())
    {
        Serial.println("Try to reconnect to  MQTT ");
        connectToMqtt();
    }
}
void Mqtt::onMqttSubscribe(uint16_t packetId, uint8_t qos)
{
    Serial.println("Subscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
    Serial.print("  qos: ");

    Serial.println(qos);
}
void Mqtt::onMqttUnsubscribe(uint16_t packetId)
{
}
void Mqtt::onMqttPublish(uint16_t packetId)
{
    Serial.println("Publish acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
}
void Mqtt::onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{

    StaticJsonDocument<600> doc;
    DeserializationError error = deserializeJson(doc, payload);
    const char *deviceName = _profile.deviceName.c_str();
    char deviceSetupTopic[32]; // devices/:name/profile
    snprintf(deviceSetupTopic, sizeof(deviceSetupTopic), DEVICE_SETUP_PROFILE_TOPIC, deviceName);

    char deviceTopic[32]; // devices/:name
    snprintf(deviceTopic, sizeof(deviceTopic), DEVICE, deviceName);

    Serial.println(strcmp(topic, deviceSetupTopic));
    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
    }
    if (strcmp(topic, "findMe") == 0)
    {
        String deviceName = doc["deviceName"];
        if (deviceName == _profile.deviceName)
        {
            _isFindMe = doc["findMe"].as<bool>();
        }
    }

    if (strcmp(topic, LOCATIONS_UPDATE) == 0)
    {
        String newLocation = doc["location"];
        _profile = _storeProfile.getProfile();
        if (newLocation == _profile.location)
        {
            _profile.city = doc["city"].as<String>();
            _storeProfile.saveProfile(_profile);
            _isUpdateProfile = true;
        }
    }
    else if (strcmp(topic, deviceSetupTopic) == 0)
    {
        Serial.println("Updating profile");

        if (_profile.location.length() > 0)
        {
            const char *deviceLocation = _profile.location.c_str();
            char deviceLocationAlarmTopic[32]; // locations/:location/alarm
            snprintf(deviceLocationAlarmTopic, sizeof(deviceLocationAlarmTopic), LOCATION_ALARM_TOPIC, deviceLocation);
            _mqttClient.unsubscribe(deviceLocationAlarmTopic);
            char deviceLocationLoggingTopic[32]; // locations/:location/logging
            snprintf(deviceLocationLoggingTopic, sizeof(deviceLocationLoggingTopic), LOCATION_LOGGING_TOPIC, deviceLocation);
            _mqttClient.unsubscribe(deviceLocationLoggingTopic);

            char deviceLocationLiveTopic[32]; // locations/:location/live
            snprintf(deviceLocationLiveTopic, sizeof(deviceLocationLiveTopic), LOCATION_LOGGING_TOPIC, deviceLocation);
            _mqttClient.unsubscribe(deviceLocationLiveTopic);

            Serial.println("removed old location");
        }

        handelSetupProfileTopic(payload, doc);

        if (_profile.location.length() > 1)
        {
            const char *deviceLocation = _profile.location.c_str();
            char deviceLocationAlarmTopic[32]; // locations/:location/alarm
            snprintf(deviceLocationAlarmTopic, sizeof(deviceLocationAlarmTopic), LOCATION_ALARM_TOPIC, deviceLocation);
            _mqttClient.subscribe(deviceLocationAlarmTopic, 1);
            char deviceLocationLoggingTopic[32]; // locations/:location/logging
            snprintf(deviceLocationLoggingTopic, sizeof(deviceLocationLoggingTopic), LOCATION_LOGGING_TOPIC, deviceLocation);
            _mqttClient.subscribe(deviceLocationLoggingTopic, 1);
            char deviceLocationLiveTopic[32]; // locations/:location/live
            snprintf(deviceLocationLiveTopic, sizeof(deviceLocationLiveTopic), LOCATION_LOGGING_TOPIC, deviceLocation);
            _mqttClient.unsubscribe(deviceLocationLiveTopic);
            Serial.println("set new location");
        }
        Serial.println("Updating profile done");
    }
    else if (strcmp(topic, UPDATE_TOPIC) == 0)
    {
        Serial.println("utopdate chacking");

        if (_profile.build == "DEV")
        {
            Serial.println("update auto  dev");
            Serial.println(_profile.isAutoUpdateOn);
            Serial.println("FW WE GOT WAS");
            int fw = doc[0]["fw"];
            Serial.println(fw);
            Serial.println("and is now");
            Serial.println(_profile.fw);

            if (_profile.isAutoUpdateOn && fw > _profile.fw)
            {
                Serial.println("auto update on DEV");
                Serial.println("new dev FW ready for download ");
                _profile.fw = fw;
                _storeProfile.saveProfile(_profile);
                profile_t _profile = _storeProfile.getProfile();
                Serial.println(_profile.mqtt_pass);
                _isDownload = true;
                _isblockProfile = true;
                // update city for new location
                StaticJsonDocument<256>
                    docSend;

                docSend["deviceName"] = _profile.deviceName;

                String msg;
                serializeJson(docSend, msg);

                _mqttClient.publish("devices", 1, false, msg.c_str());
            }
        }
        else if (_profile.build == "PROD")
        {

            Serial.println("update auto  prod");
            Serial.println(_profile.isAutoUpdateOn);
            Serial.println("FW WE GOT WAS");
            int fw = doc[1]["fw"];
            Serial.println(fw);
            Serial.println("and is now");
            Serial.println(_profile.fw);
            if (_profile.isAutoUpdateOn && fw > _profile.fw)
            {
                Serial.println("auto update on ");
                Serial.println("new prod FW ready for download ");
                _profile.fw = fw;
                _storeProfile.saveProfile(_profile);
                profile_t _profile = _storeProfile.getProfile();
                Serial.println(_profile.mqtt_pass);
                _isDownload = true;
                _isblockProfile = true;
                // update city for new location
                StaticJsonDocument<256> docSend;

                docSend["deviceName"] = _profile.deviceName;

                String msg;
                serializeJson(docSend, msg);

                _mqttClient.publish("devices", 1, false, msg.c_str());
            }
        }
    }
    doc.clear();
    Serial.println("Publish received.");
    Serial.print("  From topic: ");
    Serial.println(topic);
}
#pragma endregion
