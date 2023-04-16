#include "tempSensor.h"

TempSensor::TempSensor(RgbColor &rgb) : _rgb(rgb)
{
    _sht31 = Adafruit_SHT31();
}

void TempSensor::setup()
{
    if (!_sht31.begin(0x44))
    {
        _isTempAlarm = true;
        Serial.println("Couldn't find SHT31");
        while (1)
            delay(1);
    }

    _isTempAlarm = false;
}

void TempSensor::handelSensor()
{
    readHeater();
    readTemp();
    readHumidity();

    
    if (_isTempAlarm || _isHeaterFailure || _isHumidtyAlarm)
    {
        _isAlarmSet = true;
        _rgb.setState(ALARM);
    }
    else if (_isAlarmSet)
    {
        _isAlarmSet = false;
        _rgb.setState(ALARMOFF);
    }
}

float TempSensor::getTemp()
{
    return _currentTemp;
}

float TempSensor::getHumidity()
{
    return _currentHumidty;
}

bool TempSensor::getIsHeaterOn()
{
    return _isHeaterOn;
}

void TempSensor::setIsHeaterOn(bool heater)
{
    _isHeaterOn = heater;
}

bool TempSensor::checkForHeaterFailure(int temp)
{
    if (_isHeaterOn && _currentTemp < temp)
    {
        _isHeaterFailure = true;
        return true;
    }

    else
    {
        _isHeaterFailure = false;
        return false;
    }
}

void TempSensor::readTemp()
{
    float temp = _sht31.readTemperature();
    if (!isnan(temp))
    { // check if 'is not a number'
        _isTempAlarm = false;
        _currentTemp = temp;
    }
    else
    {
        _isTempAlarm = true;

        Serial.print("Failed to read number value from Temp sensor ");
    }
}

void TempSensor::readHeater()
{
    _sht31.heater(_isHeaterOn);
    Serial.println(_isTempAlarm);
}

void TempSensor::readHumidity()
{
    float humid = _sht31.readHumidity();
    if (!isnan(humid))
    { // check if 'is not a number'
        _isHumidtyAlarm = false;
        _currentHumidty = humid;
    }
    else
    {
        _isHumidtyAlarm = true;
        Serial.print("Failed to read number value from Humity sensor ");
    }
}
