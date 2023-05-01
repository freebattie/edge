#ifndef LIGHTSENSOR_H
#define LIGHTSENSOR_H
#include "Adafruit_LTR329_LTR303.h"
#include "rgbColor.h"

#include <WiFi.h>
#include "arduino_secrets.h"
#include "struct.h"
#include "timer.h"
class LightSensor
{
public:
    LightSensor(RgbColor &rgb);
    void setup();
    void handelSensor();
    void setLuxLevel(int lux);
    void setMinHours(int hours);
    bool getSendData();
    void setSendData(bool send);
    light_data_t getLightData();
    double getLux();

private:
    void setNoise(uint16_t ch0, uint16_t ch1);
    void calculateNoise();
    int readDay();
    float readhour();
    void readSensor();
    bool getIsAlarm();

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
    const char *_ntpServer = "0.no.pool.ntp.org";
    const long _gmtOffset_sec = 3600;
    const int _daylightOffset_sec = 3600;
    struct tm _timeinfo;
    char _getDay[3];
    char _getHour[3];
    char _getMin[3];
    float _totalSunHours;
    float _totalLampHours;
    // char _currentDay[3];
    // char _currentHour[3];
    light_data_t _lightHours;
    int _savedDay;
    int _lastHours;
    bool _isCurrentSun;
    bool _isSendData = false;
    bool _isAlarm = false;
    bool _startInitCheckTimer = false;
    Timer _sensorInitTimer;
    int _lampMaxLux;
};
#endif
