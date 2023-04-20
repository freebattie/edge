#include "roomStatus.h"

RoomStatus::RoomStatus(RgbColor &rgb) : _rgb(rgb)
{
    _lis = Adafruit_LIS3DH();
    pinMode(HALL_SENSOR_PIN, INPUT);
}

void RoomStatus::setup()
{
    _startTimer = millis();
    if (!_lis.begin(0x18))
    { // change this to 0x19 for alternative i2c address

        Serial.println("Couldnt start acc sensor");
        _rgb.setState(ALARM);
        while (1)
            yield();
    }

    _rgb.handelLight();
    delay(2000);
    _lis.setRange(LIS3DH_RANGE_2_G); // 2, 4, 8 or 16 G!
    // Adjust how often the board should read the sensor with this command
    _lis.setDataRate(LIS3DH_DATARATE_100_HZ);
    _lis.read();
    _lis.getEvent(&_event);
    while (millis() < _startTimer + 2000)
    {
        Serial.print(_startTimer + 2000 - millis());
        Serial.println(" ms untill Window Offsett is set please dont move device");
        delay(100);
    }
    _lis.read();
    _lis.getEvent(&_event);
    _windowOffsett = _event.acceleration.z;
}

void RoomStatus::handelSensors()
{

    readWindowAngelSensor();
    readDoorSensor();
}

float RoomStatus::getWindowAngel()
{
    return _windowAngel;
}

bool RoomStatus::getIsWindowOpen()
{
    Serial.print("window angel");
    Serial.println(_windowAngel);
    if (_windowAngel > 3)
        return true;

    return false;
}

bool RoomStatus::getIsDoorOpen()
{

    return _isDoorOpen;
}

void RoomStatus::readWindowAngelSensor()
{
    _lis.read();
    _lis.getEvent(&_event);
    _windowAngel = _event.acceleration.x * (float)((float)90.0 / (float)10);
}

void RoomStatus::readDoorSensor()
{
    _isDoorOpen = digitalRead(HALL_SENSOR_PIN);
}
