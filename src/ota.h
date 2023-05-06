#ifndef OTA_H
#define OTA_H

#include "InternalStorageESP.h"
#include <WiFi.h>
#include <HttpClient.h>
#include <Arduino.h>
#include "arduino_secrets.h"
#include "timer.h"
#include "struct.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "SPIFFS.h"

class Ota
{
public:
    Ota();
    void setup();
    void update(profile_t profile);
    bool initWiFi();
    bool getIsDissconnected();
    void initSPIFFS();
    void reConnect();
    void reset();
    String getMqttServerIP();
    String getMqttServerPort();
    bool setupDone();
    bool runMqttSetup();
    wl_status_t status();

private:
    static const String SERVER;
    bool _isConAlarm;
    const unsigned short SERVER_PORT = HTTP_PORT;
    const char *PATH = "/fw/%s/fw-%s-v%d.bin";
    int _Currentversion;
    bool _isAutoUpdate = false;
    static bool isSetupDone;
    static IPAddress localIP;

    // IPAddress localIP(192, 168, 1, 200); // hardcoded

    // Set your Gateway IP address
    static IPAddress localGateway;
};
#endif