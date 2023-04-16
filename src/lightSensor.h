#ifndef LIGHTSENSOR_H
#define LIGHTSENSOR_H
#include "Adafruit_LTR329_LTR303.h"
#include "rgbColor.h"
#include "time.h"
#include <WiFi.h>
#include "arduino_secrets.h"
#include "struct.h"
class LightSensor
{
public:
    LightSensor(RgbColor &rgb);
    void setup();
    void handelSensor();
    void setLuxLevel(int lux);
    void setMinHours(int hours);
    bool getSendData();
    lightData_t getLightData();
    

private:
    void setNoise(uint16_t ch0, uint16_t ch1);
    void calculateNoise();
    int readDay();
    int readhour();
    void readSensor();
    double getLux();
    bool isSunny();
    void handelSunlightLogging();
    void calculateLux();
    void calculateTotalLight(int hours);
    RgbColor &_rgb;
    Adafruit_LTR329 _ltr;
    uint16_t _visible_plus_ir, _infrared;
    int _CH0_MIN_VAL, _CH0_MAX_VAL, _CH1_MIN_VAL, _CH1_MAX_VAL, _CH0_OFFSET_VAL, _CH1_OFFSET_VAL;
    int _LuxLevel;
    int _minHours;
    double _ch0, _ch1, ratio, _lux;
    bool _valid = false;
    const char *_ntpServer = "pool.ntp.org";
    const long _gmtOffset_sec = 3600;
    const int _daylightOffset_sec = 3600;
    struct tm _timeinfo;
    char _getDay[3];
    char _getHour[3];
    //char _currentDay[3];
    //char _currentHour[3];
    lightData_t _lightHours;
    int _currentDay;
    int _currentHours;
    bool _isCurrentSun;
    bool _isSendData = false;

};
#endif
