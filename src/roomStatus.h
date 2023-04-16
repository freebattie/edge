#ifndef ROOMSTATUS_H
#define ROOMSTATUS_H
#define HALL_SENSOR_PIN 1
#include <Arduino.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include "rgbColor.h"
class RoomStatus
{
public:
    RoomStatus(RgbColor &rgb);
    void setup();
    void handelSensors();
    float getWindowAngel();
    bool getIsWindowOpen();
    bool getIsDoorOpen();

private:
    void readWindowAngelSensor();
    void readDoorSensor();
    Adafruit_LIS3DH _lis; ///< Pointer to I2C bus interface
    sensors_event_t _event;
    RgbColor &_rgb;
    bool _isAlarm = false;
    bool _isalarmSet = false;
    bool _isDoorOpen = false;
    float _windowAngel = 0; // 0 closed 90 all the way open
    float _windowOffsett = 0;
    unsigned long _startTimer;
    
};
#endif