#ifndef OTA_H
#define OTA_H

#include "InternalStorageESP.h"
#include <WiFi.h>
#include <HttpClient.h>
#include <Arduino.h>
#include "arduino_secrets.h"
#include "timer.h"

class Ota
{
public:
    Ota();
    void connectWiFi(String ssid = SECRET_SSID, String pass = SECRET_PASS);
    void reConnectWiFi(String ssid = SECRET_SSID, String pass = SECRET_PASS);
    void update(String build,int version);
    void autoUpdate(int version);
    wl_status_t status();

private:
    const char *SERVER = HTTP_SERVER;
    const unsigned short SERVER_PORT = HTTP_PORT;
    const char *PATH = "/fw/%s/fw-%s-update-v%d.bin";
    int _Currentversion;
    bool _isAutoUpdate = false;
};
#endif