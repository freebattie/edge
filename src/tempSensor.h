#ifndef TEMPSENSOR_H
#define TEMPSENSOR_H
#include <Adafruit_SHT31.h>
#include "arduino_secrets.h"
#include "rgbColor.h"

class TempSensor
{

public:
    TempSensor(RgbColor &colorController);
    void setup();
    void handelSensor();
    bool getIsAlarm();
    float getTemp();
    float getHumidity();
    bool getIsHeaterOn();
    void setIsHeaterOn(bool heater);
    bool checkForHeaterFailure(int temp);

private:
    void readTemp();
    void readHeater();
    void readHumidity();

    Adafruit_SHT31 _sht31 = NULL;
    RgbColor &_rgb;
    float _currentTemp;
    float _currentHumidty;
    bool _isHeaterOn = false;
    bool _isTempAlarm = false;
    bool _isHumidtyAlarm = false;
    bool _isHeaterFailure = false;
    bool _isAlarmSet = false;
};
#endif