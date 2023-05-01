#define NAME "edge-01"
#define DEVICETOPIC "devices/" + NAME
#define SECRET_SSID "Get-2G-72E32C"
#define SECRET_PASS "Tananger22"
// #define HTTP_SERVER "172.20.10.8"
// #define MQTT_HOST IPAddress(172, 20, 10, 8)
#define HTTP_SERVER "192.168.0.106"
#define MQTT_HOST IPAddress(192, 168, 0, 106)

// #define HTTP_SERVER "172.26.116.75"
// #define MQTT_HOST IPAddress(172, 26, 116, 75)

#define HTTP_PORT 3000

#define MQTT_PORT 4000
#define MQTT_PASSWORD ""
#define MIN_LIGH_HOURS 8
#define SUN_UP_HOUR 7
#define SUN_DOWN_HOUR 22
#define HEATER_FAILURE_TEMP 23
#define OFFSET_TEMP -16
#define OFFSET_HUMIDITY 50
#define WEATHER_API_KEY "523956835871b2c18c5357e09cbe3618"
#define FW_BUILD "dev"

#define HUMID_H_ALARM 90
#define HUMID_L_ALARM 50
#define TEMP_H_ALARM 33
#define TEMP_L_ALARM 14 // STOP GROWING

#define HUMID_H_WARN 65
#define HUMID_L_WARN 55
#define TEMP_H_WARN 30
#define TEMP_L_WARN 24

#define ON_CHANGE_SPACING 2000

#define LAMP_LUX_LEVEL 100
