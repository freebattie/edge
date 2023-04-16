#include "ota.h"

Ota::Ota()
{
}
wl_status_t Ota::status()
{
    return WiFi.status();
}
void Ota::reConnectWiFi(String ssid, String pass)
{
    WiFi.begin(ssid.c_str(), pass.c_str());
}

void Ota::connectWiFi(String ssid, String pass)
{
    Serial.println("Connecting...");
    WiFi.begin(ssid.c_str(), pass.c_str());
    Timer setupTimer = Timer();
    setupTimer.setInterval(5000);
    setupTimer.start();
    while (setupTimer.checkInterval() != RUNCODE && setupTimer.checkInterval() != STOPPED)
    {
        
        if (setupTimer.readElaspedTime() > 10000)
        {
            setupTimer.stop();
            Serial.println("WiFi connection failed");
            Serial.println("Restarting ESP");
            delay(2000);
            ESP.restart();
        }
        if (WiFi.status() != WL_CONNECTED)
        {
            WiFi.begin(ssid.c_str(), pass.c_str());
            Serial.println("Connecting...");
        }
        else
        {
            setupTimer.stop();
            Serial.println("Connected to WIFI");
        }

        delay(200);
    }
    setupTimer.stop();
}
void Ota::update(String build,int version)
{
    if (_Currentversion == version)
        return;
        
    WiFiClient wifiClient; // HTTP

    Serial.println("Connecting to http server");
    delay(500);
    Serial.print(".");
    int status = WL_IDLE_STATUS;
    HTTPClient client; // HTTP

    char buff[32];
    snprintf(buff, sizeof(buff), PATH, version);

    Serial.print("Check for update file ");
    Serial.println(buff);
    client.begin(wifiClient, SERVER, SERVER_PORT, buff);

    // Make the GET request
    int statusCode = client.GET();
    Serial.print("Update status code: ");
    Serial.println(statusCode);
    if (statusCode != 200)
    {
        client.end();
        return;
    }

    long length = client.getSize();
    if (length == -1)
    {
        client.end();
        Serial.println("Server didn't provide Content-length header. Can't continue with update.");
        return;
    }
    Serial.print("Server returned update file of size ");
    Serial.print(length);
    Serial.println(" bytes");

    if (!InternalStorage.open(length))
    {
        client.end();
        Serial.println("There is not enough space to store the update. Can't continue with update.");
        return;
    }
    byte b;
    Timer msgTimer = Timer();
    msgTimer.setInterval(1000);
    msgTimer.start();
    long fileSize = length;

    while (length > 0)
    {
        if (msgTimer.checkInterval() == RUNCODE)
        {
            msgTimer.reset();
            Serial.print("Downloading file: ");
            Serial.print(fileSize - length);
            Serial.print(" downlaoded");
            Serial.print(" of ");
            Serial.println(fileSize);
        }

        if (!client.getStream().readBytes(&b, 1)) // reading a byte with timeout
            break;
        InternalStorage.write(b);
        length--;
    }
    InternalStorage.close();
    client.end();
    if (length > 0)
    {
        Serial.print("Timeout downloading update file at ");
        Serial.print(length);
        Serial.println(" bytes. Can't continue with update.");
        return;
    }

    Serial.println("Sketch update apply and reset.");
    Serial.flush();
    InternalStorage.apply(); // this doesn't return
}
