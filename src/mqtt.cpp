#include "mqtt.h"

#pragma region member definition
const char *Mqtt::SETUP_PROFILE_PATH = "%s/profile"; // get Profile from server and save it?
const char *Mqtt::UPDATE_PATH = "update";
const char *Mqtt::SETUP_ALARM_PATH = "setup/Levels";    // alarm/warining level float
const char *Mqtt::SETUP_RUN_CAL_PATH = "setup/cal/run"; // run logging nosie/elevator/normal

bool Mqtt::_isRunLogging = true;
bool Mqtt::_isCalibratingLogging = false;
bool Mqtt::_isNoiseLogging = false;
bool Mqtt::_isUpdateProfile = false;
bool Mqtt::_isDownload = false;
bool Mqtt::_isSaveData = false;
float Mqtt::_alarmLevel;
float Mqtt::_warningLevel;
float Mqtt::_isAlarmLevel;
int Mqtt::installedFW;
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
    _storeProfile.start();
    _profile = _storeProfile.getProfile();

    WiFi.onEvent(WiFiEvent);
    _mqttClient.onConnect(onMqttConnect);
    _mqttClient.onDisconnect(onMqttDisconnect);
    _mqttClient.onSubscribe(onMqttSubscribe);
    _mqttClient.onUnsubscribe(onMqttUnsubscribe);
    _mqttClient.onMessage(onMqttMessage);
    _mqttClient.onPublish(onMqttPublish);
    _mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    Serial.println("SETUP storage in MQTT");

    Serial.println("SETUP DONE MQTT");
}

void Mqtt::connect()
{
    Serial.println("re connecting to Mqtt server");
    _mqttClient.connect();
}

void Mqtt::setIsSaveData(bool val)
{
    _isSaveData = val;
}

void Mqtt::setIsAlarmLevel(bool val)
{
    _isAlarmLevel = val;
}

bool Mqtt::getIsAlarmLevel()
{

    return _alarmLevel;
}
bool Mqtt::isUpdateFW()
{
    return _isDownload;
}
int Mqtt::getVersionToDownload()
{
    return Mqtt::installedFW;
}
bool Mqtt::isUpdateProfile()
{
    return _isUpdateProfile;
}

bool Mqtt::getIsLogging()
{
    if (_isRunLogging)
    {
        _isCalibratingLogging = false;
        _isNoiseLogging = false;
    }

    return _isRunLogging;
}
bool Mqtt::connected()
{
    return _mqttClient.connected();
}
bool Mqtt::getIsLoggingCalibrating()
{

    if (_isCalibratingLogging)
    {
        _isRunLogging = false;
        _isNoiseLogging = false;
    }

    return _isCalibratingLogging;
}
bool Mqtt::getIsNoiseCalibrating()
{
    if (_isNoiseLogging)
    {
        _isCalibratingLogging = false;
        _isRunLogging = false;
    }

    return _isNoiseLogging;
}
bool Mqtt::getIsSaveData()
{

    return _isSaveData;
}

float Mqtt::getWarningLevel()
{
    return _warningLevel;
}

float Mqtt::getAlarmLevel()
{
    return _alarmLevel;
}

profile_t Mqtt::getProfile()
{
    return _profile;
}
#pragma endregion

#pragma region private methods
void Mqtt::connectToMqtt()
{
    Serial.println("Connecting to MQTT...");
    _mqttClient.connect();
    Serial.println("MQTT CONNECTED");
}
void Mqtt::checkIfRunCalLogging(StaticJsonDocument<600> doc)
{

    String type = doc["type"];
    bool tmp = doc["run"];
    if ("cal")
    {
        if (tmp != _isCalibratingLogging && !tmp)
        {
            _isSaveData = true;
        }
        _isCalibratingLogging = doc["run"];
    }
}
void Mqtt::handelLoggingModeTopic(char *payload, StaticJsonDocument<600> doc)
{

    checkIfRunLogging(doc);
    checkIfRunNoiseLogging(doc);
    checkIfRunCalLogging(doc);
}
void Mqtt::handelSetupProfileTopic(char *payload, StaticJsonDocument<600> doc)
{
    
    _profile.deviceName = doc["deviceName"].as<String>();
    _profile.location = doc["location"].as<String>();
  
    _storeProfile.saveProfile(_profile);
    profile_t _profile = _storeProfile.getProfile();
    Serial.println(_profile.deviceName);
    _isUpdateProfile = true;
}
void Mqtt::checkIfRunLogging(StaticJsonDocument<600> doc)
{
    String type = doc["type"];
    bool tmp = doc["run"];
    if (type == "log")
    {
        if (tmp != _isRunLogging && !tmp)
        {
            _isSaveData = true;
        }
        _isRunLogging = doc["run"];
    }
}
void Mqtt::checkIfRunNoiseLogging(StaticJsonDocument<600> doc)
{
    String type = doc["type"];
    bool tmp = doc["run"];
    if (type == "noise")
    {
        if (tmp != _isNoiseLogging && !tmp)
        {
            _isSaveData = true;
        }
        _isNoiseLogging = doc["run"];
    }
}

void Mqtt::onMqttConnect(bool sessionPresent)
{
    Serial.println("connected to MQTT SERVER ");
    StaticJsonDocument<256> doc;

    // setup/calibration
    //doc["location"] = _profile.location;
    doc["deviceName"] = _profile.deviceName;
    //doc["build"] = _profile.build;
    //doc["city"] = _profile.city;
    doc["fw"] = _profile.fw;
   

    String msg;
    serializeJson(doc, msg);
    _mqttClient.publish("devices", 1, false, msg.c_str());
    doc.clear();
    const char* devicePath = _profile.deviceName.c_str();
     char deviceSetupPath[32];
    snprintf(deviceSetupPath, sizeof(deviceSetupPath),SETUP_PROFILE_PATH,devicePath);
    
    _mqttClient.subscribe("devices", 1);
    _mqttClient.subscribe(devicePath, 1);
    _mqttClient.subscribe(UPDATE_PATH, 1);
    _mqttClient.subscribe(deviceSetupPath, 1);
}
// static methods
void Mqtt::onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
    Serial.println("Disconnected from MQTT.");

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
    if (error)
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
    }

    String name = doc["name"];
    Serial.print("checking msg name ");
    Serial.print(name);
    Serial.print(" vs real name ");
    Serial.println(_profile.deviceName);
    // make sure only the device whit correct name uses new data, unless its an update to name

    if (name != _profile.deviceName && strcmp(topic, SETUP_PROFILE_PATH) != 0)
        return;

    if (strcmp(topic, SETUP_PROFILE_PATH) == 0)
    {

        Serial.println();
        handelSetupProfileTopic(payload, doc);
    }
    else if (strcmp(topic, SETUP_RUN_CAL_PATH) == 0)
    {
        handelLoggingModeTopic(payload, doc);
    }
    else if (strcmp(topic, SETUP_ALARM_PATH) == 0)
    {
        String type = doc["type"];
        if (type == "alarm")
        {
            _alarmLevel = doc["val"];
            _isAlarmLevel = true;
        }
        else if (type == "warning")
        {
            _warningLevel = doc["val"];
            _isAlarmLevel = true;
        }
    }
    else if (strcmp(topic, UPDATE_PATH) == 0)
    {
        Serial.print("got to version ");
        int version = doc["version"];
        installedFW = version;
        _isDownload = true;
    }

    Serial.println("Publish received.");
    Serial.print("  From topic: ");
    Serial.println(topic);
}
#pragma endregion
