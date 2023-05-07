#include "ota.h"
// WIFIMANAGER

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Search for parameter in HTTP POST request
const char *PARAM_INPUT_1 = "ssid";
const char *PARAM_INPUT_2 = "pass";
const char *PARAM_INPUT_3 = "ip";
const char *PARAM_INPUT_4 = "gateway";
const char *PARAM_INPUT_5 = "httpServer";
const char *PARAM_INPUT_6 = "httpPort";
const char *PARAM_INPUT_7 = "mqttServer";
const char *PARAM_INPUT_8 = "mqttPort";
// Variables to save values from HTML form

// File paths to save input values permanently
const char *ssidPath = "/ssid.txt";
const char *passPath = "/pass.txt";
const char *ipPath = "/ip.txt";
const char *gatewayPath = "/gateway.txt";
const char *statusPath = "/status.txt";
const char *httpServerPath = "/httpServer.txt";
const char *mqttServerPath = "/mqttServer.txt";
const char *httpServerPortPath = "/httpServerPort.txt";
const char *mqttServerPortPath = "/mqttServerPort.txt";
bool Ota::isSetupDone = false;
IPAddress Ota::localIP;
// IPAddress localIP(192, 168, 1, 200); // hardcoded

// Set your Gateway IP address
IPAddress Ota::localGateway;

// IPAddress localGateway(192, 168, 1, 1); //hardcoded
static IPAddress subnet(255, 255, 255, 0);
static IPAddress dns1(80, 232, 93, 176);
static IPAddress dns2(80, 232, 93, 177);
// Timer variables
static unsigned long previousMillis = 0;
static const long interval = 10000; // interval to wait for Wi-Fi connection (milliseconds)
// Set LED GPIO
static const int ledPin = 2;
// Stores LED state

static String ledState;

String _ssid;
String _pass;
String _ip;
String _gateway;
String _status;
String _mqttServer;
String _httpServer;
String _mqttServerPort;
String _httpServerPort;
String readFile(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory())
    {
        Serial.println("- failed to open file for reading");
        return String();
    }

    String fileContent;
    while (file.available())
    {
        fileContent = file.readStringUntil('\n');
        break;
    }
    return fileContent;
}
void writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- file written");
    }
    else
    {
        Serial.println("- write failed");
    }
}
String processor(const String &var)
{
    if (var == "STATE")
    {
        if (_status == "ON")
        {
            ledState = "ON";
        }
        else
        {
            ledState = "OFF";
        }
        return ledState;
    }
    return String();
}
Ota::Ota()
{
}
String Ota::getMqttServerIP()
{
    _mqttServer = readFile(SPIFFS, mqttServerPath);
    return _mqttServer;
}
String Ota::getMqttServerPort()
{
    _mqttServerPort = readFile(SPIFFS, mqttServerPortPath);
    return _mqttServerPort;
}
wl_status_t Ota::status()
{
    return WiFi.status();
}

bool Ota::setupDone()
{

    if (_status == "ON")
    {
        return false;
    }
    else if (_status == "OFF")
    {
        return true;
    }
    else
        return false;
}
bool Ota::runMqttSetup()
{
    _status = readFile(SPIFFS, statusPath);
    if (_status == "ON")
    {
        return false;
    }
    else if (_status == "OFF")
    {
        return true;
    }
    else
        return false;
}
void Ota::setup()
{
    initSPIFFS();

    // Load values saved in SPIFFS
    _ssid = readFile(SPIFFS, ssidPath);
    _pass = readFile(SPIFFS, passPath);
    _ip = readFile(SPIFFS, ipPath);
    _gateway = readFile(SPIFFS, gatewayPath);
    _status = readFile(SPIFFS, statusPath);
    _httpServer = readFile(SPIFFS, httpServerPath);
    _httpServerPort = readFile(SPIFFS, httpServerPortPath);
    _mqttServer = readFile(SPIFFS, mqttServerPath);
    _mqttServerPort = readFile(SPIFFS, mqttServerPortPath);
    Serial.println(_ssid);
    Serial.println(_pass);
    Serial.println(_ip);
    Serial.println(_gateway);
    Serial.println(_status);
    Serial.println(_httpServer);
    Serial.println(_mqttServer);
    Serial.println(_httpServerPort);
    Serial.println(_mqttServerPort);

    if (initWiFi())
    {

        // Route for root / web page
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                  { request->send(SPIFFS, "/index.html", "text/html", false, processor); });
        server.serveStatic("/", SPIFFS, "/");

        // Route to set GPIO state to HIGH
        server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
            _status = "ON";
            writeFile(SPIFFS, statusPath, _status.c_str());
            writeFile(SPIFFS, ipPath, "");
            writeFile(SPIFFS, ssidPath, "");
            delay(3000);
           
       
            request->send(200, "text/plain", "Done. ESP will restart, connect to hotspot ESP-WIFI-MANAGER and go to ip 192.168.4.1" );
            delay(3000);
            ESP.restart(); });
        // Route to set GPIO state to LOW
        server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request)
                  {
            _status = "OFF";
            writeFile(SPIFFS, statusPath, _status.c_str());
            request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + _ip); 
            delay(3000);
            ESP.restart(); });
        server.begin();
    }
    else
    {

        // Connect to Wi-Fi network with SSID and password
        Serial.println("Setting AP (Access Point)");
        // NULL sets an open Access Point
        WiFi.softAP("ESP-WIFI-MANAGER", NULL);

        IPAddress IP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(IP);

        // Web Server Root URL
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                  { request->send(SPIFFS, "/wifimanager.html", "text/html"); });

        server.serveStatic("/", SPIFFS, "/");

        server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
                  {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            _ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(_ssid);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, _ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            _pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(_pass);
            // Write file to save value
            writeFile(SPIFFS, passPath, _pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_3) {
            _ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(_ip);
            // Write file to save value
            writeFile(SPIFFS, ipPath, _ip.c_str());
            
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            _gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(_gateway);
            // Write file to save value
            writeFile(SPIFFS, gatewayPath, _gateway.c_str());
          }
          if (p->name() == PARAM_INPUT_5) {
            _httpServer = p->value().c_str();
            Serial.print("Http server is set to: ");
            Serial.println(_httpServer);
            // Write file to save value
            writeFile(SPIFFS, httpServerPath, _httpServer.c_str());
          }if (p->name() == PARAM_INPUT_6) {
            _httpServerPort = p->value().c_str();
            Serial.print("mqtt Server is set to: ");
            Serial.println(_httpServerPort);
            // Write file to save value
            writeFile(SPIFFS, httpServerPortPath, _httpServerPort.c_str());
          }
          if (p->name() == PARAM_INPUT_7) {
            _mqttServer = p->value().c_str();
            Serial.print("mqtt Server is set to: ");
            Serial.println(_mqttServer);
            // Write file to save value
            writeFile(SPIFFS, mqttServerPath, _mqttServer.c_str());
          }
          if (p->name() == PARAM_INPUT_8) {
            _mqttServerPort = p->value().c_str();
            Serial.print("mqtt Server is set to: ");
            Serial.println(_mqttServerPort);
            // Write file to save value
            writeFile(SPIFFS, mqttServerPortPath, _mqttServerPort.c_str());
          }

          
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
        request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + _ip);
        
        writeFile(SPIFFS, statusPath, "OFF");
        delay(3000);
        ESP.restart(); });
        server.begin();
    }
}
// Replaces placeholder with LED state value

void Ota::update(profile_t profile)
{

    WiFiClient wifiClient; // HTTP

    Serial.println("Connecting to http server");
    delay(500);

    int status = WL_IDLE_STATUS;
    HTTPClient client; // HTTP

    char buff[32];
    snprintf(buff, sizeof(buff), PATH, profile.build.c_str(), profile.build.c_str(), profile.fw);

    Serial.print("Check for update file ");
    Serial.println(buff);
    client.begin(wifiClient, _httpServer, _httpServerPort.toInt(), buff);

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
void Ota::reConnect()
{
    WiFi.begin(_ssid.c_str(), _pass.c_str());
}
void Ota::reset()
{
    writeFile(SPIFFS, ipPath, "");
    writeFile(SPIFFS, ssidPath, "");
    ESP.restart();
}
bool Ota::initWiFi()
{

    if (_ssid == "" || _ip == "")
    {
        Serial.println("Undefined SSID or IP address.");
        return false;
    }
    if (true)
    {
        WiFi.mode(WIFI_STA);
        localIP.fromString(_ip.c_str());
        localGateway.fromString(_gateway.c_str());

        if (!WiFi.config(localIP, localGateway, subnet, dns1, dns2))
        {
            Serial.println("STA Failed to configure");
            return false;
        }
        WiFi.begin(_ssid.c_str(), _pass.c_str());
        Serial.println("Connecting to WiFi...");

        unsigned long currentMillis = millis();
        previousMillis = currentMillis;

        while (WiFi.status() != WL_CONNECTED)
        {
            currentMillis = millis();
            if (currentMillis - previousMillis >= interval)
            {
                Serial.println("Failed to connect.");
                return false;
            }
        }

        Serial.println(WiFi.localIP());

        return true;
    }
    else
    {
        WiFi.begin(_ssid.c_str(), _pass.c_str());
        Serial.println("Connecting to WiFi...");

        unsigned long currentMillis = millis();
        previousMillis = currentMillis;

        while (WiFi.status() != WL_CONNECTED)
        {
            currentMillis = millis();
            if (currentMillis - previousMillis >= interval)
            {
                Serial.println("Failed to connect.");
                return false;
            }
        }

        Serial.println(WiFi.localIP());

        return true;
    }
}
void Ota::initSPIFFS()
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("An error has occurred while mounting SPIFFS");
    }
    Serial.println("SPIFFS mounted successfully");
}

bool Ota::getIsDissconnected()
{
    return _isConAlarm;
}
